#include "GameScene.h"
#include "MainMenuScene.h"
#include "Entities/Player/Mage.h"
#include "Entities/Enemy/KongKaZi.h"
#include "ui/CocosGUI.h"
#include <algorithm>
#include"Map/Room.h"

Scene* GameScene::createScene()
{
    return GameScene::create();
}

bool GameScene::init()
{
    if (!Scene::init())
    {
        return false;
    }
    
    _isPaused = false;
    _isGameOver = false;
    _mapGenerator = nullptr;
    _miniMap = nullptr;
    _currentRoom = nullptr;
    
    initLayers();
    initMapSystem();    // 初始化地图系统
    createPlayer();
    initCamera();       // 初始化相机
    // createTestEnemies();  // 敌人由房间生成，不再单独创建
    createHUD();
    setupKeyboardListener();
    
    // 开启update
    scheduleUpdate();
    
    GAME_LOG("GameScene initialized");
    
    return true;
}

void GameScene::initLayers()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();
    
    // 创建背景
    auto background = LayerColor::create(Color4B(40, 40, 50, 255));
    this->addChild(background, -1);
    
    // 游戏层
    _gameLayer = Layer::create();
    this->addChild(_gameLayer, Constants::ZOrder::ENTITY);
    
    // UI层
    _uiLayer = Layer::create();
    this->addChild(_uiLayer, Constants::ZOrder::UI);
}

void GameScene::initMapSystem()
{
    // 创建地图生成器
    _mapGenerator = MapGenerator::create();
    _mapGenerator->generateMap();
    _gameLayer->addChild(_mapGenerator);
    
    // 创建小地图
    _miniMap = MiniMap::create();
    _miniMap->initFromMapGenerator(_mapGenerator);
    _miniMap->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL);
    _uiLayer->addChild(_miniMap);
    
    // 设置当前房间
    _currentRoom = _mapGenerator->getBeginRoom();
    
    GAME_LOG("Map system initialized with %d rooms", _mapGenerator->getRoomCount());
}

void GameScene::initCamera()
{
    // 不创建自定义相机，使用默认相机
    // 通过移动游戏层来实现相机跟随效果
    _camera = nullptr;
    
    GAME_LOG("Camera system initialized (using default camera with layer movement)");
}

void GameScene::updateCamera(float dt)
{
    // 玩家死亡或不存在时不更新相机
    if (!_player || _player->isDead() || !_mapGenerator) return;
    
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 playerPos = _player->getPosition();
    
    // 计算地图总边界（所有房间的范围）
    float minX = FLT_MAX, maxX = -FLT_MAX;
    float minY = FLT_MAX, maxY = -FLT_MAX;
    
    // 遍历所有房间，找到地图边界
    for (int y = 0; y < Constants::MAP_GRID_SIZE; y++) {
        for (int x = 0; x < Constants::MAP_GRID_SIZE; x++) {
            Room* room = _mapGenerator->getRoom(x, y);
            if (room) {
                Vec2 center = room->getCenter();
                float roomWidth = Constants::ROOM_TILES_W * Constants::FLOOR_TILE_SIZE;
                float roomHeight = Constants::ROOM_TILES_H * Constants::FLOOR_TILE_SIZE;
                
                minX = std::min(minX, center.x - roomWidth / 2.0f);
                maxX = std::max(maxX, center.x + roomWidth / 2.0f);
                minY = std::min(minY, center.y - roomHeight / 2.0f);
                maxY = std::max(maxY, center.y + roomHeight / 2.0f);
            }
        }
    }
    
    // 获取屏幕尺寸的一半（可见范围）
    float halfWidth = visibleSize.width / 2.0f;
    float halfHeight = visibleSize.height / 2.0f;
    
    // 计算游戏层应该的位置（相机跟随的反向）
    // 玩家在屏幕中心，所以游戏层要向相反方向移动
    float targetX = halfWidth - playerPos.x;
    float targetY = halfHeight - playerPos.y;
    
    // 限制游戏层位置，使其不超出地图边界
    if (maxX - minX > visibleSize.width) {
        // 地图宽度大于屏幕
        float minLayerX = visibleSize.width - maxX;
        float maxLayerX = -minX;
        targetX = std::max(minLayerX, std::min(maxLayerX, targetX));
    } else {
        // 地图宽度小于屏幕，居中显示
        targetX = halfWidth - (minX + maxX) / 2.0f;
    }
    
    if (maxY - minY > visibleSize.height) {
        // 地图高度大于屏幕
        float minLayerY = visibleSize.height - maxY;
        float maxLayerY = -minY;
        targetY = std::max(minLayerY, std::min(maxLayerY, targetY));
    } else {
        // 地图高度小于屏幕，居中显示
        targetY = halfHeight - (minY + maxY) / 2.0f;
    }
    
    // 平滑移动游戏层
    Vec2 currentPos = _gameLayer->getPosition();
    float smoothFactor = 0.1f;
    
    Vec2 newPos;
    newPos.x = currentPos.x + (targetX - currentPos.x) * smoothFactor;
    newPos.y = currentPos.y + (targetY - currentPos.y) * smoothFactor;
    
    _gameLayer->setPosition(newPos);
}

void GameScene::createPlayer()
{
    // 创建法师角色
    _player = Mage::create();
    
    if (_player == nullptr) {
        GAME_LOG_ERROR("Failed to create Mage!");
        return;
    }
    
    // 设置玩家初始位置在起始房间的中心
    if (_currentRoom) {
        Vec2 roomCenter = _currentRoom->getCenter();
        _player->setPosition(roomCenter);
        GAME_LOG("Player created at room center (%.1f, %.1f)", roomCenter.x, roomCenter.y);
        GAME_LOG("Room walkable area: X[%.1f, %.1f], Y[%.1f, %.1f]",
                 _currentRoom->getWalkableArea().getMinX(),
                 _currentRoom->getWalkableArea().getMaxX(),
                 _currentRoom->getWalkableArea().getMinY(),
                 _currentRoom->getWalkableArea().getMaxY());
    } else {
        // 如果没有当前房间，使用屏幕中心作为后备
        _player->setPosition(SCREEN_CENTER);
        GAME_LOG("Player created at screen center (%.1f, %.1f)", 
                 SCREEN_CENTER.x, SCREEN_CENTER.y);
    }
    
    // 设置玩家的全局Z顺序
    _player->setGlobalZOrder(Constants::ZOrder::ENTITY);
    
    _gameLayer->addChild(_player);
}

void GameScene::createTestEnemies()
{
    // 创建3个测试敌人
    for (int i = 0; i < 3; i++)
    {
        auto enemy = Enemy::create();
        
        // 随机位置
        float randomX = RANDOM_FLOAT(100, SCREEN_WIDTH - 100);
        float randomY = RANDOM_FLOAT(100, SCREEN_HEIGHT - 100);
        enemy->setPosition(Vec2(randomX, randomY));
        
        // 创建敌人精灵（临时使用纯色方块）
        auto enemySprite = Sprite::create();
        auto drawNode = DrawNode::create();
        drawNode->drawSolidRect(Vec2(-20, -20), Vec2(20, 20), Color4F::RED);
        enemySprite->addChild(drawNode);
        enemy->bindSprite(enemySprite);
        
        _gameLayer->addChild(enemy);
        _enemies.pushBack(enemy);
        
        // 测试红色标记
        enemy->tryApplyRedMark(0.3f);
        
        GAME_LOG("Enemy %d created at position (%.1f, %.1f)", i, randomX, randomY);
    }
}

void GameScene::spawnEnemiesInRoom(Room* room)
{
    if (room == nullptr)
    {
        return;
    }
    
    // 只在普通战斗房间生成敌人
    if (room->getRoomType() != Constants::RoomType::NORMAL)
    {
        return;
    }
    
    // 如果房间已经生成过敌人，则不再生成
    if (room->isEnemiesSpawned())
    {
        return;
    }
    
    // 标记已生成敌人
    room->setEnemiesSpawned(true);
    
    // 关闭房间的门
    room->closeDoors();
    
    // 获取房间中心和尺寸
    Vec2 roomCenter = room->getCenter();
    float roomWidth = Constants::ROOM_TILES_W * Constants::FLOOR_TILE_SIZE;
    float roomHeight = Constants::ROOM_TILES_H * Constants::FLOOR_TILE_SIZE;
    
    // 计算房间边界（用于限制敌人移动）
    Rect roomBounds = Rect(
        roomCenter.x - roomWidth * 0.4f,
        roomCenter.y - roomHeight * 0.4f,
        roomWidth * 0.8f,
        roomHeight * 0.8f
    );
    
    // 随机生成2-4个阿咬
    int enemyCount = RANDOM_INT(2, 4);
    
    for (int i = 0; i < enemyCount; i++)
    {
        auto ayao = Ayao::create();
        
        // 在房间内随机位置（避开房间边界）
        float offsetX = RANDOM_FLOAT(-roomWidth * 0.3f, roomWidth * 0.3f);
        float offsetY = RANDOM_FLOAT(-roomHeight * 0.3f, roomHeight * 0.3f);
        Vec2 spawnPos = roomCenter + Vec2(offsetX, offsetY);
        
        ayao->setPosition(spawnPos);
        ayao->setRoomBounds(roomBounds);  // 设置房间边界限制移动
        
        // 添加到场景和全局敌人列表
        _gameLayer->addChild(ayao);
        _enemies.pushBack(ayao);
        
        // 同时添加到房间的敌人列表（用于房间门控制）
        room->getEnemies().pushBack(ayao);
        
        // 添加 ayao 到场景并 pushback 后：
        ayao->tryApplyRedMark(0.3f);
        
        GAME_LOG("Ayao spawned at (%.1f, %.1f) in room", spawnPos.x, spawnPos.y);
    }
}

void GameScene::createHUD()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();
    
    float barStartX = origin.x + 60;
    float barStartY = origin.y + visibleSize.height - 35;
    
    // ==================== 血条 ====================
    float barWidth = 120.0f;  // 血条宽度
    float barHeight = 12.0f;  // 血条高度
    
    // 爱心图标
    _hpIcon = Sprite::create("UIs/StatusBars/Bars/Heart.png");
    _hpIcon->setPosition(Vec2(barStartX, barStartY));
    _hpIcon->setScale(0.12f);
    _hpIcon->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL);
    _uiLayer->addChild(_hpIcon);
    
    // 血条背景
    auto hpBarBg = Sprite::create("UIs/StatusBars/Bars/EmplyBar.png");
    hpBarBg->setPosition(Vec2(barStartX + 25, barStartY));
    hpBarBg->setAnchorPoint(Vec2(0, 0.5f));
    hpBarBg->setScaleX(barWidth / hpBarBg->getContentSize().width);
    hpBarBg->setScaleY(barHeight / hpBarBg->getContentSize().height);
    hpBarBg->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL);
    _uiLayer->addChild(hpBarBg);
    
    // 血条填充
    _hpBar = cocos2d::ui::LoadingBar::create("UIs/StatusBars/Bars/HealthFill.png");
    _hpBar->setPercent(100.0f);
    _hpBar->setPosition(Vec2(barStartX + 25, barStartY));
    _hpBar->setAnchorPoint(Vec2(0, 0.5f));
    _hpBar->setScaleX(barWidth / _hpBar->getContentSize().width);
    _hpBar->setScaleY(barHeight / _hpBar->getContentSize().height);
    _hpBar->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 5);
    _uiLayer->addChild(_hpBar);
    
    // 血量数值
    _hpLabel = Label::createWithSystemFont("100/100", "Arial", 16);
    _hpLabel->setPosition(Vec2(barStartX + 75, barStartY));
    _hpLabel->setTextColor(Color4B::WHITE);
    _hpLabel->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 10);
    _uiLayer->addChild(_hpLabel);
    
    // ==================== 蓝条 ====================
    float mpBarY = barStartY - 35;
    
    // 闪电图标
    _mpIcon = Sprite::create("UIs/StatusBars/Bars/Lighting bolt.png");
    _mpIcon->setPosition(Vec2(barStartX, mpBarY));
    _mpIcon->setScale(0.12f);
    _mpIcon->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL);
    _uiLayer->addChild(_mpIcon);
    
    // 蓝条背景
    auto mpBarBg = Sprite::create("UIs/StatusBars/Bars/EmplyBar.png");
    mpBarBg->setPosition(Vec2(barStartX + 25, mpBarY));
    mpBarBg->setAnchorPoint(Vec2(0, 0.5f));
    mpBarBg->setScaleX(barWidth / mpBarBg->getContentSize().width);
    mpBarBg->setScaleY(barHeight / mpBarBg->getContentSize().height);
    mpBarBg->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL);
    _uiLayer->addChild(mpBarBg);
    
    // 蓝条填充
    _mpBar = cocos2d::ui::LoadingBar::create("UIs/StatusBars/Bars/EnrgyFill.png");
    _mpBar->setPercent(100.0f);
    _mpBar->setPosition(Vec2(barStartX + 25, mpBarY));
    _mpBar->setAnchorPoint(Vec2(0, 0.5f));
    _mpBar->setScaleX(barWidth / _mpBar->getContentSize().width);
    _mpBar->setScaleY(barHeight / _mpBar->getContentSize().height);
    _mpBar->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 5);
    _uiLayer->addChild(_mpBar);
    
    // 蓝量数值
    _mpLabel = Label::createWithSystemFont("100/100", "Arial", 16);
    _mpLabel->setPosition(Vec2(barStartX + 75, mpBarY));
    _mpLabel->setTextColor(Color4B::WHITE);
    _mpLabel->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 10);
    _uiLayer->addChild(_mpLabel);
    
    // ==================== 技能图标CD系统 ====================
    float skillIconSize = 64.0f;
    float skillIconX = origin.x + visibleSize.width - skillIconSize / 2 - 40;
    float skillIconY = origin.y + skillIconSize / 2 + 40;  // 下方治疗技能位置
    float skillIconY2 = skillIconY + skillIconSize + 10;    // 上方角色技能位置
    
    // ====== 上方：角色特殊技能（K键） ======
    _skillIcon = Sprite::create("UIs/Skills/Mage/Nymph_skillicon.png");
    _skillIcon->setPosition(Vec2(skillIconX, skillIconY2));
    _skillIcon->setScale(skillIconSize / _skillIcon->getContentSize().width);
    _skillIcon->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL);
    _uiLayer->addChild(_skillIcon);
    
    // CD变暗遮罩
    _skillCDMask = Sprite::create("UIs/Skills/Mage/Nymph_skillicon.png");
    _skillCDMask->setPosition(Vec2(skillIconX, skillIconY2));
    _skillCDMask->setScale(skillIconSize / _skillCDMask->getContentSize().width);
    _skillCDMask->setColor(Color3B::BLACK);
    _skillCDMask->setOpacity(100);
    _skillCDMask->setVisible(false);
    _skillCDMask->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 1);
    _uiLayer->addChild(_skillCDMask);
    
    // CD进度条
    auto progressSprite = Sprite::create("UIs/Skills/Mage/Nymph_skillicon.png");
    _skillCDProgress = ProgressTimer::create(progressSprite);
    _skillCDProgress->setType(ProgressTimer::Type::RADIAL);
    _skillCDProgress->setReverseDirection(false);
    _skillCDProgress->setMidpoint(Vec2(0.5f, 0.5f));
    _skillCDProgress->setBarChangeRate(Vec2(1, 1));
    _skillCDProgress->setPosition(Vec2(skillIconX, skillIconY2));
    _skillCDProgress->setScale(skillIconSize / progressSprite->getContentSize().width);
    _skillCDProgress->setPercentage(0.0f);
    _skillCDProgress->setColor(Color3B(100, 100, 100));
    _skillCDProgress->setOpacity(150);
    _skillCDProgress->setVisible(false);
    _skillCDProgress->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 2);
    _uiLayer->addChild(_skillCDProgress);
    
    // ====== 下方：治疗技能（L键） ======
    _healIcon = Sprite::create("UIs/Skills/Healing.png");
    _healIcon->setPosition(Vec2(skillIconX, skillIconY));
    _healIcon->setScale(skillIconSize / _healIcon->getContentSize().width);
    _healIcon->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL);
    _uiLayer->addChild(_healIcon);
    
    // 治疗CD变暗遮罩
    _healCDMask = Sprite::create("UIs/Skills/Healing.png");
    _healCDMask->setPosition(Vec2(skillIconX, skillIconY));
    _healCDMask->setScale(skillIconSize / _healCDMask->getContentSize().width);
    _healCDMask->setColor(Color3B::BLACK);
    _healCDMask->setOpacity(100);
    _healCDMask->setVisible(false);
    _healCDMask->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 1);
    _uiLayer->addChild(_healCDMask);
    
    // 治疗CD进度条
    auto healProgressSprite = Sprite::create("UIs/Skills/Healing.png");
    _healCDProgress = ProgressTimer::create(healProgressSprite);
    _healCDProgress->setType(ProgressTimer::Type::RADIAL);
    _healCDProgress->setReverseDirection(false);
    _healCDProgress->setMidpoint(Vec2(0.5f, 0.5f));
    _healCDProgress->setBarChangeRate(Vec2(1, 1));
    _healCDProgress->setPosition(Vec2(skillIconX, skillIconY));
    _healCDProgress->setScale(skillIconSize / healProgressSprite->getContentSize().width);
    _healCDProgress->setPercentage(0.0f);
    _healCDProgress->setColor(Color3B(100, 100, 100));
    _healCDProgress->setOpacity(150);
    _healCDProgress->setVisible(false);
    _healCDProgress->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 2);
    _uiLayer->addChild(_healCDProgress);
    
    // Debug信息
    _debugLabel = Label::createWithSystemFont("", "Arial", 18);
    _debugLabel->setPosition(Vec2(origin.x + visibleSize.width - 150, origin.y + visibleSize.height - 30));
    _debugLabel->setTextColor(Color4B::YELLOW);
    _debugLabel->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL);
    _uiLayer->addChild(_debugLabel);
    
    // 操作提示
    auto hintLabel = Label::createWithTTF(
        u8"操作说明：\nWASD - 移动\nJ - 攻击\nK - 技能\nL - 治疗\n空格 - 冲刺\nESC - 暂停",
        "fonts/msyh.ttf", 18);
    hintLabel->setPosition(Vec2(origin.x + 120, origin.y + 120));
    hintLabel->setTextColor(Color4B::WHITE);
    hintLabel->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL);
    _uiLayer->addChild(hintLabel);
}

void GameScene::update(float dt)
{
    if (_isPaused)
    {
        return;
    }
    
    Scene::update(dt);
    
    updatePlayer(dt);
    updateCamera(dt);     // 更新相机位置
    updateMapSystem(dt);  // 更新地图系统
    updateEnemies(dt);
    updateHUD(dt);
    checkCollisions();
}

void GameScene::updateMapSystem(float dt)
{
    // 玩家死亡或不存在时不更新地图系统
    if (_mapGenerator == nullptr || _player == nullptr || _player->isDead()) return;
    
    // 使用玩家的本地坐标（相对于_gameLayer）
    Vec2 playerPos = _player->getPosition();
    
    // 检测玩家当前所在房间
    Room* detectedRoom = _mapGenerator->updatePlayerRoom(_player);
    
    if (detectedRoom != nullptr && detectedRoom != _currentRoom)
    {
        // 玩家进入了新房间
        _currentRoom = detectedRoom;
        
        // 在新房间生成敌人
        spawnEnemiesInRoom(_currentRoom);
        
        // 更新小地图
        if (_miniMap)
        {
            _miniMap->updateCurrentRoom(_currentRoom);
        }
        
        GAME_LOG("Player entered room (%d, %d)", 
                 _currentRoom->getGridX(), 
                 _currentRoom->getGridY());
    }
    
    // 边界检测和修正
    float tileSize = Constants::FLOOR_TILE_SIZE;
    bool positionCorrected = false;
    Vec2 correctedPos = playerPos;
    
    // 遍历所有房间检测
    bool inAnyRoom = false;
    for (auto room : _mapGenerator->getAllRooms()) {
        if (room == nullptr) continue;
        
        Rect walkable = room->getWalkableArea();
        Vec2 center = room->getCenter();
        float doorHalfWidth = Constants::DOOR_WIDTH * tileSize / 2.0f;
        
        // 检查是否在房间扩展范围内
        float extLeft = walkable.getMinX() - tileSize;
        float extRight = walkable.getMaxX() + tileSize;
        float extTop = walkable.getMaxY() + tileSize;
        float extBottom = walkable.getMinY() - tileSize;
        
        if (playerPos.x >= extLeft && playerPos.x <= extRight &&
            playerPos.y >= extBottom && playerPos.y <= extTop) {
            
            inAnyRoom = true;
            bool canPassDoor = room->allEnemiesKilled();
            
            // 检测并修正左边界
            if (playerPos.x < walkable.getMinX()) {
                bool canPassLeft = canPassDoor && room->hasDoor(Constants::DIR_LEFT) && 
                                   std::abs(playerPos.y - center.y) <= doorHalfWidth;
                if (!canPassLeft) {
                    correctedPos.x = walkable.getMinX();
                    positionCorrected = true;
                }
            }
            // 检测并修正右边界
            if (playerPos.x > walkable.getMaxX()) {
                bool canPassRight = canPassDoor && room->hasDoor(Constants::DIR_RIGHT) && 
                                    std::abs(playerPos.y - center.y) <= doorHalfWidth;
                if (!canPassRight) {
                    correctedPos.x = walkable.getMaxX();
                    positionCorrected = true;
                }
            }
            // 检测并修正下边界
            if (playerPos.y < walkable.getMinY()) {
                bool canPassDown = canPassDoor && room->hasDoor(Constants::DIR_DOWN) && 
                                   std::abs(playerPos.x - center.x) <= doorHalfWidth;
                if (!canPassDown) {
                    correctedPos.y = walkable.getMinY();
                    positionCorrected = true;
                }
            }
            // 检测并修正上边界
            if (playerPos.y > walkable.getMaxY()) {
                bool canPassUp = canPassDoor && room->hasDoor(Constants::DIR_UP) && 
                                 std::abs(playerPos.x - center.x) <= doorHalfWidth;
                if (!canPassUp) {
                    correctedPos.y = walkable.getMaxY();
                    positionCorrected = true;
                }
            }
            
            break;  // 找到所在房间后退出
        }
    }
    
    // 如果不在任何房间内，检测走廊
    if (!inAnyRoom) {
        for (auto hallway : _mapGenerator->getAllHallways()) {
            if (hallway == nullptr) continue;
            
            Rect bounds = hallway->getWalkableArea();
            int dir = hallway->getDirection();
            
            // 扩展检测范围
            float extLeft = bounds.getMinX() - tileSize;
            float extRight = bounds.getMaxX() + tileSize;
            float extTop = bounds.getMaxY() + tileSize;
            float extBottom = bounds.getMinY() - tileSize;
            
            if (playerPos.x >= extLeft && playerPos.x <= extRight &&
                playerPos.y >= extBottom && playerPos.y <= extTop) {
                
                // 根据走廊方向限制
                if (dir == Constants::DIR_LEFT || dir == Constants::DIR_RIGHT) {
                    // 水平走廊：限制Y方向
                    if (playerPos.y > bounds.getMaxY()) {
                        correctedPos.y = bounds.getMaxY();
                        positionCorrected = true;
                    }
                    if (playerPos.y < bounds.getMinY()) {
                        correctedPos.y = bounds.getMinY();
                        positionCorrected = true;
                    }
                }
                else {
                    // 垂直走廊：限制X方向
                    if (playerPos.x > bounds.getMaxX()) {
                        correctedPos.x = bounds.getMaxX();
                        positionCorrected = true;
                    }
                    if (playerPos.x < bounds.getMinX()) {
                        correctedPos.x = bounds.getMinX();
                        positionCorrected = true;
                    }
                }
                
                break;  // 找到所在走廊后退出
            }
        }
    }
    
    // 应用位置修正
    if (positionCorrected) {
        _player->setPosition(correctedPos);
    }
}

void GameScene::updatePlayer(float dt)
{
    if (_player == nullptr)
    {
        return;
    }
    
    // 检查玩家是否死亡
    if (_player->isDead())
    {
        // 只显示一次游戏结束界面
        if (!_isGameOver)
        {
            _isGameOver = true;
            showGameOver();
        }
        return;
    }
}

void GameScene::updateEnemies(float dt)
{
    // 游戏结束后不再更新敌人
    if (_isGameOver)
    {
        return;
    }
    
    // 玩家死亡后不再更新敌人AI
    if (_player == nullptr || _player->isDead())
    {
        return;
    }
    
    // 更新敌人AI
    for (auto enemy : _enemies)
    {
        if (enemy != nullptr && !enemy->isDead())
        {
            enemy->executeAI(_player, dt);
        }
    }
    
    // 移除死亡的敌人
    for (auto it = _enemies.begin(); it != _enemies.end(); )
    {
        if (*it == nullptr || (*it)->isDead())
        {
            it = _enemies.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void GameScene::updateHUD(float dt)
{
    if (_player == nullptr)
    {
        return;
    }
    
    // 玩家死亡时显示HP为0
    int currentHP = _player->isDead() ? 0 : _player->getHP();
    int maxHP = _player->getMaxHP();
    float hpPercent = (maxHP > 0) ? (currentHP * 100.0f / maxHP) : 0.0f;
    _hpBar->setPercent(hpPercent);
    
    char hpText[32];
    sprintf(hpText, "%d/%d", currentHP, maxHP);
    _hpLabel->setString(hpText);
        
        // 更新MP蓝条
        int currentMP = _player->getMP();
        int maxMP = _player->getMaxMP();
        float mpPercent = (maxMP > 0) ? (currentMP * 100.0f / maxMP) : 0.0f;
        _mpBar->setPercent(mpPercent);
        
        char mpText[32];
        sprintf(mpText, "%d/%d", currentMP, maxMP);
        _mpLabel->setString(mpText);
        
        // 更新技能冷却
        float remain = _player->getSkillCooldownRemaining();
        float totalCD = _player->getSkillCooldown();
        
        if (remain <= 0.0f)
        {
            // 技能可用：正常显示，隐藏遮罩和进度
            _skillIcon->setOpacity(255);
            _skillCDMask->setVisible(false);
            _skillCDProgress->setVisible(false);
        }
        else
        {
            // 技能CD中：图标轻微变暗，显示转圈进度
            _skillIcon->setOpacity(200);
            _skillCDMask->setVisible(true);
            _skillCDProgress->setVisible(true);
            
            // 计算CD进度（从100%到0%，顺时针消失）
            float cdPercent = (totalCD > 0) ? ((totalCD - remain) / totalCD * 100.0f) : 0.0f;
            _skillCDProgress->setPercentage(100.0f - cdPercent);
        }
        
        // 更新治疗技能冷却
        float healRemain = _player->getHealCooldownRemaining();
        float healTotalCD = _player->getHealCooldown();
        
        if (healRemain <= 0.0f)
        {
            // 治疗可用：正常显示，隐藏遮罩和进度
            _healIcon->setOpacity(255);
            _healCDMask->setVisible(false);
            _healCDProgress->setVisible(false);
        }
        else
        {
            // 治疗CD中：图标轻微变暗，显示转圈进度
            _healIcon->setOpacity(200);
            _healCDMask->setVisible(true);
            _healCDProgress->setVisible(true);
            
            // 计算CD进度
            float healCdPercent = (healTotalCD > 0) ? ((healTotalCD - healRemain) / healTotalCD * 100.0f) : 0.0f;
            _healCDProgress->setPercentage(100.0f - healCdPercent);
        }
        
        // 更新Debug信息
        char debugText[128];
        const char* roomTypeStr = "Unknown";
        if (_currentRoom) {
            switch (_currentRoom->getRoomType()) {
                case Constants::RoomType::BEGIN: roomTypeStr = "Start"; break;
                case Constants::RoomType::NORMAL: roomTypeStr = "Normal"; break;
                case Constants::RoomType::BOSS: roomTypeStr = "Boss"; break;
                case Constants::RoomType::END: roomTypeStr = "End"; break;
                case Constants::RoomType::WEAPON: roomTypeStr = "Weapon"; break;
                case Constants::RoomType::PROP: roomTypeStr = "Prop"; break;
                default: break;
            }
        }
        sprintf(debugText, "Room: %s\nRooms: %d\nPos: (%.0f, %.0f)", 
                roomTypeStr,
                _mapGenerator ? _mapGenerator->getRoomCount() : 0,
                _player->getPositionX(),
                _player->getPositionY());
        _debugLabel->setString(debugText);
}

void GameScene::checkCollisions()
{
    if (_player == nullptr || _player->isDead())
    {
        return;
    }
    
    // 玩家攻击判定 - 检测玩家攻击范围内的敌人
    if (_player->getState() == EntityState::ATTACK)
    {
        for (auto enemy : _enemies)
        {
            if (enemy != nullptr && !enemy->isDead())
            {
                // 简单的距离判定
                float dist = _player->getPosition().distance(enemy->getPosition());
                if (dist < 80.0f)  // 攻击范围
                {
                    enemy->takeDamage(_player->getAttack());
                    GAME_LOG("Player hits enemy for %d damage!", _player->getAttack());
                }
            }
        }
    }
    
    // 玩家技能判定（火球等） - 在Mage::castFireball中处理
}

void GameScene::setupKeyboardListener()
{
    auto listener = EventListenerKeyboard::create();
    
    listener->onKeyPressed = [this](EventKeyboard::KeyCode keyCode, Event* event) {
        if (keyCode == EventKeyboard::KeyCode::KEY_ESCAPE)
        {
            if (_isPaused)
            {
                resumeGame();
            }
            else
            {
                pauseGame();
            }
        }
    };
    
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);
}

void GameScene::pauseGame()
{
    _isPaused = true;
    
    // 停止玩家输入和更新
    if (_player) {
        _player->removeInputEvents();
        _player->pause();
    }
    
    // 停止所有敌人
    for (auto enemy : _enemies) {
        if (enemy) {
            enemy->pause();
        }
    }
    
    // 暂停游戏层更新（包括相机等）
    _gameLayer->pause();
    
    // 添加半透明遮罩
    auto mask = LayerColor::create(Color4B(0, 0, 0, 180));
    mask->setName("pauseMask");
    mask->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL);
    _uiLayer->addChild(mask);
    
    // 创建暂停菜单UI - 直接添加到_uiLayer，使用绝对位置
    // 标题
    auto titleLabel = Label::createWithTTF(u8"游戏已暂停", "fonts/msyh.ttf", 56);
    titleLabel->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y + 120));
    titleLabel->setTextColor(Color4B::WHITE);
    titleLabel->setName("pauseTitle");
    titleLabel->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 1);
    _uiLayer->addChild(titleLabel);
    
    // 继续游戏按钮
    auto resumeButton = ui::Button::create();
    resumeButton->setTitleText(u8"继续游戏");
    resumeButton->setTitleFontName("fonts/msyh.ttf");
    resumeButton->setTitleFontSize(32);
    resumeButton->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y + 20));
    resumeButton->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 1);
    resumeButton->setName("pauseResumeBtn");
    resumeButton->addClickEventListener([this](Ref* sender) {
        resumeGame();
    });
    _uiLayer->addChild(resumeButton);
    
    // 返回主菜单按钮
    auto menuButton = ui::Button::create();
    menuButton->setTitleText(u8"返回主菜单");
    menuButton->setTitleFontName("fonts/msyh.ttf");
    menuButton->setTitleFontSize(32);
    menuButton->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y - 60));
    menuButton->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 1);
    menuButton->setName("pauseMenuBtn");
    menuButton->addClickEventListener([](Ref* sender) {
        auto menuScene = MainMenuScene::createScene();
        Director::getInstance()->replaceScene(TransitionFade::create(0.5f, menuScene));
    });
    _uiLayer->addChild(menuButton);
    
    // 退出游戏按钮
    auto exitButton = ui::Button::create();
    exitButton->setTitleText(u8"退出游戏");
    exitButton->setTitleFontName("fonts/msyh.ttf");
    exitButton->setTitleFontSize(32);
    exitButton->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y - 140));
    exitButton->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 1);
    exitButton->setName("pauseExitBtn");
    exitButton->addClickEventListener([](Ref* sender) {
        Director::getInstance()->end();
    });
    _uiLayer->addChild(exitButton);
    
    GAME_LOG("Game paused");
}

void GameScene::resumeGame()
{
    _isPaused = false;
    
    // 恢复玩家输入和更新
    if (_player) {
        _player->registerInputEvents();
        _player->resume();
    }
    
    // 恢复所有敌人
    for (auto enemy : _enemies) {
        if (enemy) {
            enemy->resume();
        }
    }
    
    // 恢复游戏层更新
    _gameLayer->resume();
    
    // 移除暂停菜单UI元素
    _uiLayer->removeChildByName("pauseTitle");
    _uiLayer->removeChildByName("pauseResumeBtn");
    _uiLayer->removeChildByName("pauseMenuBtn");
    _uiLayer->removeChildByName("pauseExitBtn");
    _uiLayer->removeChildByName("pauseMask");
    
    GAME_LOG("Game resumed");
}

void GameScene::showGameOver()
{
    GAME_LOG("Game Over!");
    
    // 停止游戏更新
    this->unscheduleUpdate();
    
    // 停止玩家输入
    if (_player != nullptr)
    {
        _player->removeInputEvents();
        _player->stopAllActions();
    }
    
    // 停止所有敌人
    for (auto enemy : _enemies)
    {
        if (enemy != nullptr)
        {
            enemy->stopAllActions();
            enemy->unscheduleAllCallbacks();
        }
    }
    
    // 添加半透明遮罩
    auto mask = LayerColor::create(Color4B(0, 0, 0, 180));
    mask->setName("gameOverMask");
    mask->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL);
    _uiLayer->addChild(mask);
    
    // 显示游戏结束文字
    auto gameOverLabel = Label::createWithTTF(u8"游戏结束", "fonts/msyh.ttf", 64);
    gameOverLabel->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y + 50));
    gameOverLabel->setTextColor(Color4B::RED);
    gameOverLabel->setName("gameOverLabel");
    gameOverLabel->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 1);
    _uiLayer->addChild(gameOverLabel);
    
    // 显示重新开始提示
    auto hintLabel = Label::createWithTTF(u8"按 R 重新开始\n按 Q 退出到主菜单", "fonts/msyh.ttf", 32);
    hintLabel->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y - 50));
    hintLabel->setTextColor(Color4B::WHITE);
    hintLabel->setName("gameOverHint");
    hintLabel->setAlignment(TextHAlignment::CENTER);
    hintLabel->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 1);
    _uiLayer->addChild(hintLabel);
    
    // 添加重新开始的键盘监听 - 使用this捕获
    auto listener = EventListenerKeyboard::create();
    listener->onKeyPressed = [this](EventKeyboard::KeyCode keyCode, Event* event) {
        if (keyCode == EventKeyboard::KeyCode::KEY_R)
        {
            // 重新开始
            auto newScene = GameScene::createScene();
            Director::getInstance()->replaceScene(TransitionFade::create(0.5f, newScene));
        }
        else if (keyCode == EventKeyboard::KeyCode::KEY_Q)
        {
            // 返回主菜单
            auto menuScene = MainMenuScene::createScene();
            Director::getInstance()->replaceScene(TransitionFade::create(0.5f, menuScene));
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);
}

void GameScene::addEnemy(Enemy* enemy)
{
    if (enemy == nullptr) return;

    // 添加到游戏层（显示）并注册到 _enemies（用于 AI 更新）
    if (_gameLayer)
    {
        enemy->setGlobalZOrder(Constants::ZOrder::ENTITY);
        _gameLayer->addChild(enemy);
    }
    else
    {
        this->addChild(enemy);
    }

    enemy->setTag(Constants::Tag::ENEMY);

    bool already = false;
    for (auto e : _enemies)
    {
        if (e == enemy) { already = true; break; }
    }
    if (!already)
    {
        _enemies.pushBack(enemy);
    }

    // --- 新增：尝试将该敌人归入所在房间的房间敌人列表，并设置房间边界 ---
    if (_mapGenerator)
    {
        auto rooms = _mapGenerator->getAllRooms();
        for (auto room : rooms)
        {
            if (!room) continue;

            // 使用房间可行走区判断敌人位置是否属于该房间
            Rect walk = room->getWalkableArea();
            if (walk.containsPoint(enemy->getPosition()))
            {
                // 将敌人加入房间管理列表（避免重复）
                bool inRoom = false;
                for (auto re : room->getEnemies())
                {
                    if (re == enemy) { inRoom = true; break; }
                }
                if (!inRoom)
                {
                    room->getEnemies().pushBack(enemy);
                }

                // 计算与 spawnEnemiesInRoom 相同的活动范围（0.8 * 房间尺寸，中心为 room->getCenter）
                Vec2 center = room->getCenter();
                float roomWidth = Constants::ROOM_TILES_W * Constants::FLOOR_TILE_SIZE;
                float roomHeight = Constants::ROOM_TILES_H * Constants::FLOOR_TILE_SIZE;
                Rect roomBounds = Rect(
                    center.x - roomWidth * 0.4f,
                    center.y - roomHeight * 0.4f,
                    roomWidth * 0.8f,
                    roomHeight * 0.8f
                );

                // 将边界传给敌人（如果子类实现，会应用）
                enemy->setRoomBounds(roomBounds);

                GAME_LOG("GameScene::addEnemy - enemy assigned to room (%d,%d) and bounds set",
                         room->getGridX(), room->getGridY());
                break;
            }
        }
    }
    // --- end 新增 ---

    GAME_LOG("GameScene::addEnemy - enemy added and registered (total=%zu)", _enemies.size());
}