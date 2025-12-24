#include "GameScene.h"
#include "MainMenuScene.h"
#include "Entities/Player/Mage.h"

// 静态变量定义
int GameScene::s_nextLevel = 1;
int GameScene::s_nextStage = 1;
int GameScene::s_savedHP = 0;
int GameScene::s_savedMP = 0;
#include "Entities/Enemy/KongKaZi.h"
#include "Entities/Enemy/DeYi.h"
#include "Entities/Enemy/Ayao.h"
#include "Entities/Enemy/XinXing.h"
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
    _keyE = false;
    _mapGenerator = nullptr;
    _miniMap = nullptr;
    _currentRoom = nullptr;
    _interactionLabel = nullptr;
    
    // 初始化关卡系统（从静态变量读取）
    _currentLevel = s_nextLevel;
    _currentStage = s_nextStage;
    _savedHP = s_savedHP;
    _savedMP = s_savedMP;
    
    GAME_LOG("GameScene init: Level %d-%d, SavedHP: %d, SavedMP: %d", 
             _currentLevel, _currentStage, _savedHP, _savedMP);
    
    // 重置静态变量为默认值（防止影响后续的重新开始）
    s_nextLevel = 1;
    s_nextStage = 1;
    s_savedHP = 0;
    s_savedMP = 0;
    
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
    _miniMap->updateLevelDisplay(_currentLevel, _currentStage);
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
    
    // 如果有保存的血蓝量，恢复之
    if (_savedHP > 0) {
        _player->setHP(_savedHP);
        _player->setMP(_savedMP);
        GAME_LOG("Restored player HP: %d, MP: %d", _savedHP, _savedMP);
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
    
    // 获取房间中心和尺寸（roomCenter 仍可用于日志等）
    Vec2 roomCenter = room->getCenter();
    
    // 使用房间自身计算的可行走区域（基于墙与玩家半径），确保边界与墙对齐
    Rect walk = room->getWalkableArea();
    
    // 随机生成3-8个怪（Ayao / DeYi / XinXing）
    int enemyCount = RANDOM_INT(3, 8);

    for (int i = 0; i < enemyCount; i++)
    {
        Enemy* enemy = nullptr;
        float r = CCRANDOM_0_1();
        // 概率分配：40% Ayao, 50% DeYi, 10% XinXing (精英)
        if (r < 0.3f)
        {
            enemy = Ayao::create();
        }
        else if (r < 0.9f)
        {
            enemy = DeYi::create();
        }
        else
        {
            enemy = XinXing::create();
        }

        if (!enemy) continue;

        // 在 walk 可行走区域内随机位置（注意：walk 是绝对坐标，直接采样）
        float spawnX = RANDOM_FLOAT(walk.getMinX() + 1.0f, walk.getMaxX() - 1.0f);
        float spawnY = RANDOM_FLOAT(walk.getMinY() + 1.0f, walk.getMaxY() - 1.0f);
        Vec2 spawnPos = Vec2(spawnX, spawnY);

        enemy->setPosition(spawnPos);
        // 通过统一入口设置房间边界并将敌人注册到场景/列表中
        enemy->setRoomBounds(walk);

        // ---------- 新增：对每个新生成的敌人按基础速度的 90%~110% 随机化移动速度 ----------
        // 保持子类/基础初始化的默认值不变（只是对该实例做微调）
        float baseSpeed = enemy->getMoveSpeed(); // 子类在 init() 里已设置基础速度
        float randFactor = RANDOM_FLOAT(0.9f, 1.1f);
        enemy->setMoveSpeed(baseSpeed * randFactor);
        GAME_LOG("Enemy speed randomized: base=%.1f factor=%.3f final=%.1f", baseSpeed, randFactor, enemy->getMoveSpeed());
        // ------------------------------------------------------------------------------

        // 使用 GameScene::addEnemy 统一注册（会加入场景、_enemies，并尝试将其加入对应房间）
        addEnemy(enemy);

        // 尝试应用红色标记（30%）
        enemy->tryApplyRedMark(0.3f);

        const char* typeName = "Unknown";
        if (dynamic_cast<Ayao*>(enemy)) typeName = "Ayao";
        else if (dynamic_cast<DeYi*>(enemy)) typeName = "DeYi";
        else if (dynamic_cast<XinXing*>(enemy)) typeName = "XinXing";

        GAME_LOG("Enemy spawned at (%.1f, %.1f) in room - type=%s", spawnPos.x, spawnPos.y, typeName);
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
    updateSpikes(dt);     // 更新地刺伤害
    updateInteraction(dt); // 更新交互提示
    updateHUD(dt);
    checkBarrierCollisions(); // 检测障碍物碰撞
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

void GameScene::updateSpikes(float dt)
{
    if (_isGameOver)
    {
        return;
    }

    if (_player == nullptr || _player->isDead())
    {
        return;
    }

    if (_currentRoom == nullptr)
    {
        return;
    }

    const auto& spikes = _currentRoom->getSpikes();
    for (auto spike : spikes)
    {
        if (!spike) continue;
        spike->updateState(dt, _player);
    }
}

void GameScene::updateInteraction(float dt)
{
    if (!_player || !_currentRoom)
    {
        if (_interactionLabel && _interactionLabel->isVisible())
        {
            _interactionLabel->setVisible(false);
        }
        return;
    }
    
    // 优先检测传送门交互
    bool canInteractPortal = _currentRoom->canInteractWithPortal(_player);
    if (canInteractPortal)
    {
        Sprite* portal = _currentRoom->getPortal();
        if (portal)
        {
            if (!_interactionLabel)
            {
                // 创建交互提示标签
                _interactionLabel = Label::createWithTTF(u8"[E] 进入传送门", "fonts/msyh.ttf", 20);
                _interactionLabel->setTextColor(Color4B::YELLOW);
                _interactionLabel->enableOutline(Color4B::BLACK, 2);
                _interactionLabel->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 5);
                _uiLayer->addChild(_interactionLabel);
            }
            else
            {
                _interactionLabel->setString(u8"[E] 进入传送门");
            }
            
            // 将传送门的世界坐标转换为屏幕坐标
            Vec2 portalWorldPos = portal->getParent()->convertToWorldSpace(portal->getPosition());
            Vec2 screenPos = _uiLayer->convertToNodeSpace(portalWorldPos);
            
            // 显示在传送门下方
            float portalHeight = portal->getContentSize().height * portal->getScale();
            _interactionLabel->setPosition(Vec2(screenPos.x, screenPos.y - portalHeight * 0.6f));
            _interactionLabel->setVisible(true);
            return;
        }
    }
    
    // 检测是否可以与宝箱交互
    bool canInteractChest = _currentRoom->canInteractWithChest(_player);
    
    // 显示或隐藏交互提示
    if (canInteractChest)
    {
        Sprite* chest = _currentRoom->getChest();
        if (!chest)
        {
            if (_interactionLabel && _interactionLabel->isVisible())
            {
                _interactionLabel->setVisible(false);
            }
            return;
        }
        
        if (!_interactionLabel)
        {
            // 创建交互提示标签
            _interactionLabel = Label::createWithTTF(u8"[E] 打开宝箱", "fonts/msyh.ttf", 20);
            _interactionLabel->setTextColor(Color4B::YELLOW);
            _interactionLabel->enableOutline(Color4B::BLACK, 2);
            _interactionLabel->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 5);
            _uiLayer->addChild(_interactionLabel);
        }
        else
        {
            _interactionLabel->setString(u8"[E] 打开宝箱");
        }
        
        // 将宝箱的世界坐标转换为屏幕坐标
        Vec2 chestWorldPos = chest->getParent()->convertToWorldSpace(chest->getPosition());
        Vec2 screenPos = _uiLayer->convertToNodeSpace(chestWorldPos);
        
        // 显示在宝箱下方
        float chestHeight = chest->getContentSize().height * chest->getScale();
        _interactionLabel->setPosition(Vec2(screenPos.x, screenPos.y - chestHeight * 0.6f));
        _interactionLabel->setVisible(true);
    }
    else
    {
        if (_interactionLabel && _interactionLabel->isVisible())
        {
            _interactionLabel->setVisible(false);
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
                case Constants::RoomType::REWARD: roomTypeStr = "Reward"; break;
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

void GameScene::checkBarrierCollisions()
{
    if (!_currentRoom)
    {
        return;
    }
    
    const auto& barriers = _currentRoom->getBarriers();
    if (barriers.empty())
    {
        return;
    }
    
    // 检测玩家与障碍物碰撞
    if (_player && !_player->isDead())
    {
        Vec2 playerPos = _player->getPosition();
        
        // 获取玩家碰撞箱（使用精灵大小的40%作为碰撞体积，让玩家更灵活）
        Size playerSize = Size(32, 32); 
        if (_player->getSprite()) {
             playerSize = _player->getSprite()->getBoundingBox().size;
             playerSize.width *= 0.4f;
             playerSize.height *= 0.4f;
        }
        
        Rect playerBox(playerPos.x - playerSize.width / 2, 
                       playerPos.y - playerSize.height / 2, 
                       playerSize.width, 
                       playerSize.height);
        
        for (auto barrier : barriers)
        {
            if (!barrier || !barrier->blocksMovement())
            {
                continue;
            }
            
            // 障碍物在Room节点下，Room节点在(0,0)，所以障碍物的getBoundingBox就是世界坐标（相对于GameLayer）
            Rect barrierBox = barrier->getBoundingBox();
            
            // 缩小障碍物碰撞箱一点点，避免卡住
            float shrink = 2.0f;
            barrierBox.origin.x += shrink;
            barrierBox.origin.y += shrink;
            barrierBox.size.width -= shrink * 2;
            barrierBox.size.height -= shrink * 2;
            
            if (playerBox.intersectsRect(barrierBox))
            {
                // 发生碰撞，计算推出方向
                Vec2 barrierPos = barrier->getPosition();
                Vec2 delta = playerPos - barrierPos;
                
                // 计算重叠量
                float overlapX = (playerBox.size.width + barrierBox.size.width) / 2.0f - abs(delta.x);
                float overlapY = (playerBox.size.height + barrierBox.size.height) / 2.0f - abs(delta.y);
                
                // 沿重叠较小的方向推出
                if (overlapX < overlapY)
                {
                    // 水平方向推出
                    if (delta.x > 0)
                    {
                        _player->setPositionX(playerPos.x + overlapX);
                    }
                    else
                    {
                        _player->setPositionX(playerPos.x - overlapX);
                    }
                }
                else
                {
                    // 垂直方向推出
                    if (delta.y > 0)
                    {
                        _player->setPositionY(playerPos.y + overlapY);
                    }
                    else
                    {
                        _player->setPositionY(playerPos.y - overlapY);
                    }
                }
                
                // 更新位置后重新获取
                playerPos = _player->getPosition();
            }
        }
    }
    
    // 检测敌人与障碍物碰撞
    for (auto enemy : _enemies)
    {
        if (!enemy || enemy->isDead())
        {
            continue;
        }
        
        Vec2 enemyPos = enemy->getPosition();
        
        // 获取敌人碰撞箱
        Size enemySize = Size(32, 32); 
        if (enemy->getSprite()) {
             enemySize = enemy->getSprite()->getBoundingBox().size;
             enemySize.width *= 0.6f;
             enemySize.height *= 0.6f;
        }
        
        Rect enemyBox(enemyPos.x - enemySize.width / 2, 
                      enemyPos.y - enemySize.height / 2, 
                      enemySize.width, 
                      enemySize.height);
        
        for (auto barrier : barriers)
        {
            if (!barrier || !barrier->blocksMovement())
            {
                continue;
            }
            
            Rect barrierBox = barrier->getBoundingBox();
            // 缩小障碍物碰撞箱
            float shrink = 2.0f;
            barrierBox.origin.x += shrink;
            barrierBox.origin.y += shrink;
            barrierBox.size.width -= shrink * 2;
            barrierBox.size.height -= shrink * 2;
            
            if (enemyBox.intersectsRect(barrierBox))
            {
                // 发生碰撞，计算推出方向
                Vec2 barrierPos = barrier->getPosition();
                Vec2 delta = enemyPos - barrierPos;
                
                // 计算重叠量
                float overlapX = (enemyBox.size.width + barrierBox.size.width) / 2.0f - abs(delta.x);
                float overlapY = (enemyBox.size.height + barrierBox.size.height) / 2.0f - abs(delta.y);
                
                // 沿重叠较小的方向推出
                if (overlapX < overlapY)
                {
                    // 水平方向推出
                    if (delta.x > 0)
                    {
                        enemy->setPositionX(enemyPos.x + overlapX);
                    }
                    else
                    {
                        enemy->setPositionX(enemyPos.x - overlapX);
                    }
                }
                else
                {
                    // 垂直方向推出
                    if (delta.y > 0)
                    {
                        enemy->setPositionY(enemyPos.y + overlapY);
                    }
                    else
                    {
                        enemy->setPositionY(enemyPos.y - overlapY);
                    }
                }
                
                // 更新位置
                enemyPos = enemy->getPosition();
            }
        }
    }
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
        else if (keyCode == EventKeyboard::KeyCode::KEY_E)
        {
            _keyE = true;
            // 处理交互：传送门优先
            if (_currentRoom && _currentRoom->canInteractWithPortal(_player))
            {
                goToNextLevel();
            }
            // 处理交互：宝箱
            else if (_currentRoom && _currentRoom->canInteractWithChest(_player))
            {
                _currentRoom->openChest();
            }
        }
    };
    
    listener->onKeyReleased = [this](EventKeyboard::KeyCode keyCode, Event* event) {
        if (keyCode == EventKeyboard::KeyCode::KEY_E)
        {
            _keyE = false;
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
    resumeButton->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y + 60));
    resumeButton->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 1);
    resumeButton->setName("pauseResumeBtn");
    resumeButton->addClickEventListener([this](Ref* sender) {
        resumeGame();
    });
    _uiLayer->addChild(resumeButton);
    
    // 设置按钮
    auto settingsButton = ui::Button::create();
    settingsButton->setTitleText(u8"设置");
    settingsButton->setTitleFontName("fonts/msyh.ttf");
    settingsButton->setTitleFontSize(32);
    settingsButton->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y - 10));
    settingsButton->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 1);
    settingsButton->setName("pauseSettingsBtn");
    settingsButton->addClickEventListener([this](Ref* sender) {
        showSettings();
    });
    _uiLayer->addChild(settingsButton);
    
    // 返回主菜单按钮
    auto menuButton = ui::Button::create();
    menuButton->setTitleText(u8"返回主菜单");
    menuButton->setTitleFontName("fonts/msyh.ttf");
    menuButton->setTitleFontSize(32);
    menuButton->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y - 80));
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
    exitButton->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y - 150));
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
    _uiLayer->removeChildByName("pauseSettingsBtn");
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

void GameScene::showVictory()
{
    GAME_LOG("Victory!");
    
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
    mask->setName("victoryMask");
    mask->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL);
    _uiLayer->addChild(mask);
    
    // 显示胜利文字
    auto victoryLabel = Label::createWithTTF(u8"通关成功！", "fonts/msyh.ttf", 64);
    victoryLabel->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y + 50));
    victoryLabel->setTextColor(Color4B::YELLOW);
    victoryLabel->setName("victoryLabel");
    victoryLabel->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 1);
    _uiLayer->addChild(victoryLabel);
    
    // 显示提示
    auto hintLabel = Label::createWithTTF(u8"按 R 重新开始\n按 Q 退出到主菜单", "fonts/msyh.ttf", 32);
    hintLabel->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y - 50));
    hintLabel->setTextColor(Color4B::WHITE);
    hintLabel->setName("victoryHint");
    hintLabel->setAlignment(TextHAlignment::CENTER);
    hintLabel->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 1);
    _uiLayer->addChild(hintLabel);
    
    // 添加键盘监听
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

void GameScene::goToNextLevel()
{
    if (!_player)
    {
        return;
    }
    
    // 保存当前血蓝量
    int savedHP = _player->getHP();
    int savedMP = _player->getMP();
    
    // 进入下一小关
    int nextLevel = _currentLevel;
    int nextStage = _currentStage + 1;
    
    GAME_LOG("Going to next level. Current: %d-%d, Next: %d-%d, Saving HP: %d, MP: %d", 
             _currentLevel, _currentStage, nextLevel, nextStage, savedHP, savedMP);
    
    // 判断是否通关（1-2 后结束）
    if (nextLevel == 1 && nextStage > 2)
    {
        // 显示胜利界面
        showVictory();
        return;
    }
    
    // 使用静态变量传递信息给新场景
    s_nextLevel = nextLevel;
    s_nextStage = nextStage;
    s_savedHP = savedHP;
    s_savedMP = savedMP;
    
    GAME_LOG("Set static vars for next scene: Level %d-%d, HP: %d, MP: %d", 
             nextLevel, nextStage, savedHP, savedMP);
    
    // 创建新场景（init时会读取静态变量）
    auto newScene = GameScene::createScene();
    if (newScene)
    {
        // 切换场景
        Director::getInstance()->replaceScene(TransitionFade::create(0.5f, newScene));
    }
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
                // 将敌人加入房间管理列表（避免重复），但只有在 countsForRoomClear() == true 时才计入
                if (enemy->countsForRoomClear())
                {
                    bool inRoom = false;
                    for (auto re : room->getEnemies())
                    {
                        if (re == enemy) { inRoom = true; break; }
                    }
                    if (!inRoom)
                    {
                        room->getEnemies().pushBack(enemy);
                    }
                }

                // 将 Room::getWalkableArea 直接传给敌人，保证边界与房间墙匹配
                enemy->setRoomBounds(walk);

                GAME_LOG("GameScene::addEnemy - enemy assigned to room (%d,%d) and bounds set",
                         room->getGridX(), room->getGridY());
                break;
            }
        }
    }
    // --- end 新增 ---

    // 确保动态生成的敌人也有机会成为“红色标记”怪以生成 KongKaZi
    // spawnEnemiesInRoom 已对初生群体调用 tryApplyRedMark，但通过 addEnemy 动态加入的（如 XinXing 死后生成的 IronLance）也需要调用
    enemy->tryApplyRedMark(0.3f);

    GAME_LOG("GameScene::addEnemy - enemy added and registered (total=%zu)", _enemies.size());
}

void GameScene::showSettings()
{
    GAME_LOG("Opening settings menu");
    
    // 隐藏暂停菜单
    _uiLayer->getChildByName("pauseTitle")->setVisible(false);
    _uiLayer->getChildByName("pauseResumeBtn")->setVisible(false);
    _uiLayer->getChildByName("pauseSettingsBtn")->setVisible(false);
    _uiLayer->getChildByName("pauseMenuBtn")->setVisible(false);
    _uiLayer->getChildByName("pauseExitBtn")->setVisible(false);
    
    // 创建设置层
    auto settingsLayer = SettingsLayer::create();
    settingsLayer->setCloseCallback([this]() {
        // 恢复显示暂停菜单
        _uiLayer->getChildByName("pauseTitle")->setVisible(true);
        _uiLayer->getChildByName("pauseResumeBtn")->setVisible(true);
        _uiLayer->getChildByName("pauseSettingsBtn")->setVisible(true);
        _uiLayer->getChildByName("pauseMenuBtn")->setVisible(true);
        _uiLayer->getChildByName("pauseExitBtn")->setVisible(true);
    });
    _uiLayer->addChild(settingsLayer);
}