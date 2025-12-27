#include "GameScene.h"
#include "MainMenuScene.h"
#include "Entities/Player/Mage.h"
#include "Entities/Player/Gunner.h"
#include "Entities/Player/Warrior.h"
#include "UI/CharacterSelectLayer.h"
#include "Entities/Enemy/KongKaZi.h"
#include "Entities/Enemy/DeYi.h"
#include "Entities/Enemy/Ayao.h"
#include "Entities/Enemy/XinXing.h"
#include "Entities/Enemy/TangHuang.h"
#include "Entities/Enemy/Du.h"
#include "Entities/Enemy/Cup.h"
#include "Entities/Enemy/Boat.h"
#include "Entities/Enemy/KuiLongBoss.h"
#include "Entities/Objects/Chest.h"
#include "Entities/Objects/ItemDrop.h"
#include "ui/CocosGUI.h"
#include "audio/include/AudioEngine.h"
#include <algorithm>
#include "Map/Room.h"

// 静态变量定义
int GameScene::s_nextLevel = 1;
int GameScene::s_nextStage = 1;
int GameScene::s_savedHP = 0;
int GameScene::s_savedMP = 0;
std::vector<std::string> GameScene::s_savedItems;

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
    _gameHUD = nullptr;
    _gameMenus = nullptr;
    _settingsLayer = nullptr;
    
    // 初始化关卡系统（从静态变量读取）
    _currentLevel = s_nextLevel;
    _currentStage = s_nextStage;
    _savedHP = s_savedHP;
    _savedMP = s_savedMP;
    _collectedItems = s_savedItems;  // 恢复已收集的道具列表
    
    GAME_LOG("GameScene init: Level %d-%d, SavedHP: %d, SavedMP: %d, SavedItems: %d", 
             _currentLevel, _currentStage, _savedHP, _savedMP, (int)_collectedItems.size());
    
    // 重置静态变量为默认值（防止影响后续的重新开始）
    s_nextLevel = 1;
    s_nextStage = 1;
    s_savedHP = 0;
    s_savedMP = 0;
    s_savedItems.clear();
    
    initLayers();
    initMapSystem();    // 初始化地图系统
    createPlayer();
    initCamera();       // 初始化相机
    // createTestEnemies();  // 敌人由房间生成，不再单独创建
    createHUD();
    
    // 恢复上一关收集的道具
    if (!_collectedItems.empty())
    {
        GAME_LOG("Restoring %d items from previous level", (int)_collectedItems.size());
        const auto& allItems = ItemLibrary::all();
        for (const auto& itemId : _collectedItems)
        {
            // 根据ID查找道具定义
            for (const auto& item : allItems)
            {
                if (item.id == itemId)
                {
                    // 恢复道具到UI（注意不要再次添加到_collectedItems）
                    if (_gameHUD)
                    {
                        _gameHUD->addItemIcon(&item);
                    }
                    
                    // 重新应用道具效果到玩家（不包括一次性治疗效果）
                    if (_player)
                    {
                        ItemLibrary::applyItemEffect(itemId, _player);
                    }
                    
                    GAME_LOG("Restored item: %s", item.name.c_str());
                    break;
                }
            }
        }
    }
    
    // 道具效果恢复后，恢复保存的HP/MP（这样可以正确反映道具对MaxHP的影响）
    if (_player && _savedHP > 0) {
        _player->setHP(_savedHP);
        _player->setMP(_savedMP);
        GAME_LOG("Restored player HP: %d, MP: %d (after item effects)", _savedHP, _savedMP);
    }
    
    createMenus();
    setupKeyboardListener();
    
    // 开启update
    scheduleUpdate();
    
    // 播放战斗音乐
    AudioEngine::stopAll();
    if (_currentStage == 0)
    {
        // Boss战斗音乐
        AudioEngine::play2d("Music/Boss_Battle.mp3", true);
        GAME_LOG("Playing Boss battle music");
    }
    else
    {
        // 普通战斗音乐
        AudioEngine::play2d("Music/Game_Battle.mp3", true);
        GAME_LOG("Playing normal battle music");
    }
    
    GAME_LOG("GameScene initialized");
    
    return true;
}

void GameScene::initLayers()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();
    
    // 创建背景（深绿色）
    auto background = LayerColor::create(Color4B(20, 60, 30, 255));
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
    _mapGenerator->setLevelNumber(_currentLevel, _currentStage);
    
    // stage=0表示Boss层
    if (_currentStage == 0) {
        _mapGenerator->setBossFloor(true);
    }
    
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
                // 使用房间实际尺寸，支持Boss房间2倍大小
                float roomWidth = room->getTilesWidth() * Constants::FLOOR_TILE_SIZE;
                float roomHeight = room->getTilesHeight() * Constants::FLOOR_TILE_SIZE;
                
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
    // 根据角色选择创建对应的角色
    CharacterType selectedCharacter = CharacterSelectLayer::getSelectedCharacter();
    
    switch (selectedCharacter)
    {
        case CharacterType::MAGE:
            _player = Mage::create();
            GAME_LOG("Creating Mage (Nymph)");
            break;
        case CharacterType::GUNNER:
            _player = Gunner::create();
            GAME_LOG("Creating Gunner (Wisdael)");
            break;
        case CharacterType::WARRIOR:
            _player = Warrior::create();
            GAME_LOG("Creating Warrior (Mudrock)");
            break;
        default:
            _player = Mage::create();
            GAME_LOG("Creating default Mage");
            break;
    }
    
    if (_player == nullptr) {
        GAME_LOG_ERROR("Failed to create player!");
        return;
    }
    
    // 注意：HP/MP恢复移到道具效果恢复之后，在init()中进行
    
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
    if (room == nullptr) return;
    
    // 只在普通战斗房间和Boss房间生成敌人
    if (room->getRoomType() != Constants::RoomType::NORMAL && 
        room->getRoomType() != Constants::RoomType::BOSS)
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
    
    // Boss房间：只生成一个Boss在中心
    if (room->getRoomType() == Constants::RoomType::BOSS)
    {
        auto boss = KuiLongBoss::create();
        if (boss)
        {
            boss->setPosition(roomCenter);
            boss->setRoomBounds(walk);
            
            // 获取三阶段房间并设置给 Boss
            // 假设 MapGenerator 已经生成了 BossFloor，我们需要找到它
            // 这里通过遍历子节点找到 BossFloor，或者通过 MapGenerator 获取
            // 由于 GameScene 持有 _mapGenerator，我们可以尝试从那里获取信息
            // 但 MapGenerator 没有直接暴露 BossFloor，我们需要一种方式获取 Phase3Room
            
            // 方案：遍历所有房间，找到位于 Boss 房间右侧的那个房间
            if (_mapGenerator) {
                int bossGridX = room->getGridX();
                int bossGridY = room->getGridY();
                Room* phase3Room = _mapGenerator->getRoom(bossGridX + 1, bossGridY);
                if (phase3Room) {
                    boss->setPhase3Room(phase3Room);
                }
            }

            addEnemy(boss);
            GAME_LOG("KuiLongBoss spawned at boss room center (%.1f, %.1f)", roomCenter.x, roomCenter.y);
        }

        // 2.1: Boss房间初始会生成30个怪，怪物只包含妒，阿咬，魂灵圣杯和堂皇。
        int initialMinions = 30;
        for (int i = 0; i < initialMinions; ++i) {
            Enemy* minion = nullptr;
            float r = CCRANDOM_0_1();
            // 四种怪物均分概率 (各25%)
            if (r < 0.25f) minion = Du::create();
            else if (r < 0.50f) minion = Ayao::create();
            else if (r < 0.75f) minion = Cup::create();
            else minion = TangHuang::create();

            if (minion) {
                // 在可行走区域内随机生成
                float x = walk.origin.x + CCRANDOM_0_1() * walk.size.width;
                float y = walk.origin.y + CCRANDOM_0_1() * walk.size.height;
                minion->setPosition(Vec2(x, y));
                addEnemy(minion);
            }
        }
        return; 
    }

    // 随机生成3-8个怪（普通小怪：Ayao / DeYi，精英怪：XinXing / TangHuang / Du）
    int enemyCount = RANDOM_INT(3, 8);

    for (int i = 0; i < enemyCount; i++)
    {
        Enemy* enemy = nullptr;
        float r = CCRANDOM_0_1();
        // 目标：Cup + Du 合计 30%，其余 70% 平均给 Ayao/DeYi/XinXing/TangHuang（每个 17.5%）
        if (r < 0.30f)
        {
            // 在 30% 区间内等概率选择 Cup 或 Du（各 ~15%）
            float r2 = CCRANDOM_0_1();
            if (r2 < 0.5f) {
                enemy = Cup::create();
            } else {
                enemy = Du::create();
            }
        }
        else
        {
            // 将剩余区间归一化为 0..1，平均分 4 份
            float r2 = (r - 0.30f) / 0.70f; // 0..1
            if (r2 < 0.25f) {
                enemy = Ayao::create();
            } else if (r2 < 0.50f) {
                enemy = DeYi::create();
            } else if (r2 < 0.75f) {
                enemy = XinXing::create();
            } else {
                enemy = TangHuang::create();
            }
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
        else if (dynamic_cast<TangHuang*>(enemy)) typeName = "TangHuang";
        else if (dynamic_cast<Du*>(enemy)) typeName = "Du";
        else if (dynamic_cast<Cup*>(enemy)) typeName = "Cup"; // 新增：Cup 的类型名

        GAME_LOG("Enemy spawned at (%.1f, %.1f) in room - type=%s", spawnPos.x, spawnPos.y, typeName);
    }
}

void GameScene::createHUD()
{
    _gameHUD = GameHUD::create();
    _gameHUD->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL);
    _uiLayer->addChild(_gameHUD);
    
    // 根据选择的角色设置技能图标
    CharacterType selectedCharacter = CharacterSelectLayer::getSelectedCharacter();
    _gameHUD->setSkillIcon(static_cast<int>(selectedCharacter));
    
    GAME_LOG("GameHUD created");
}

void GameScene::createMenus()
{
    _gameMenus = GameMenus::create();
    _gameMenus->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 10);
    _uiLayer->addChild(_gameMenus);
    
    // 设置回调
    _gameMenus->setResumeCallback([this]() {
        resumeGame();
    });
    
    _gameMenus->setSettingsCallback([this]() {
        showSettings();
    });
    
    _gameMenus->setRestartCallback([this]() {
        auto newScene = GameScene::createScene();
        Director::getInstance()->replaceScene(TransitionFade::create(0.5f, newScene));
    });
    
    _gameMenus->setMainMenuCallback([this]() {
        auto menuScene = MainMenuScene::createScene();
        Director::getInstance()->replaceScene(TransitionFade::create(0.5f, menuScene));
    });
    
    GAME_LOG("GameMenus created");
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
    
    // 检测boss房间胜利条件
    if (_currentRoom && _currentRoom->getRoomType() == Constants::RoomType::BOSS)
    {
        if (_currentRoom->allEnemiesKilled())
        {
            GAME_LOG("Boss room cleared! Showing victory.");
            showVictory();
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
    if (!_player || !_currentRoom || !_gameHUD)
    {
        if (_gameHUD) {
            _gameHUD->hideInteractionHint();
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
            Vec2 portalWorldPos = portal->getParent()->convertToWorldSpace(portal->getPosition());
            float portalHeight = portal->getContentSize().height * portal->getScale();
            _gameHUD->showInteractionHint(u8"[E] 进入传送门", portalWorldPos, portalHeight * 0.6f);
            return;
        }
    }
    
    // 检测是否可以与宝箱或道具交互
    bool canInteractChest = _currentRoom->canInteractWithChest(_player);
    bool canInteractItem = _currentRoom->canInteractWithItemDrop(_player);
    
    // 显示或隐藏交互提示（道具优先于宝箱）
    if (canInteractItem)
    {
        ItemDrop* itemDrop = _currentRoom->getItemDrop();
        if (!itemDrop || !itemDrop->getItemDef())
        {
            _gameHUD->hideInteractionHint();
            return;
        }
        
        const ItemDef* itemDef = itemDrop->getItemDef();
        Sprite* itemSprite = itemDrop->getSprite();
        if (!itemSprite)
        {
            _gameHUD->hideInteractionHint();
            return;
        }
        
        std::string interactText = std::string(u8"[E]获取") + itemDef->name + u8"：" + itemDef->description;
        CCLOG("Showing item interaction: %s", interactText.c_str());
        
        Vec2 itemWorldPos = itemDrop->getParent()->convertToWorldSpace(itemDrop->getPosition());
        float itemHeight = itemSprite->getContentSize().height * itemSprite->getScale();
        _gameHUD->showInteractionHint(interactText, itemWorldPos, itemHeight * 0.6f);
    }
    else if (canInteractChest)
    {
        Chest* chestObj = _currentRoom->getChest();
        if (!chestObj)
        {
            _gameHUD->hideInteractionHint();
            return;
        }
        
        Sprite* chest = chestObj->getSprite();
        if (!chest)
        {
            _gameHUD->hideInteractionHint();
            return;
        }
        
        Vec2 chestWorldPos = chest->getParent()->convertToWorldSpace(chest->getPosition());
        float chestHeight = chest->getContentSize().height * chest->getScale();
        _gameHUD->showInteractionHint(u8"[E] 打开宝箱", chestWorldPos, chestHeight * 0.6f);
    }
    else
    {
        _gameHUD->hideInteractionHint();
    }
}

void GameScene::updateHUD(float dt)
{
    if (_player == nullptr || _gameHUD == nullptr)
    {
        return;
    }
    
    int roomCount = _mapGenerator ? _mapGenerator->getRoomCount() : 0;
    _gameHUD->update(_player, _currentRoom, roomCount);
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
        
        // 使用固定的碰撞体积，与角色视觉大小无关
        // 这样所有角色都能通过相同大小的间隙
        float collisionSize = Constants::FLOOR_TILE_SIZE * 1.5f;  // 约1.5格的碰撞体积
        Size playerSize = Size(collisionSize, collisionSize);
        
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
            // 如果设置层打开，先关闭设置层
            if (_settingsLayer)
            {
                _settingsLayer->close();
                return;
            }
            
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
            // 处理交互：拾取道具
            else if (_currentRoom && _currentRoom->canInteractWithItemDrop(_player))
            {
                const ItemDef* itemDef = _currentRoom->pickupItemDrop(_player);
                if (itemDef)
                {
                    addItemToUI(itemDef);
                }
            }
            // 处理交互：宝箱
            else if (_currentRoom && _currentRoom->canInteractWithChest(_player))
            {
                _currentRoom->openChest(_player);
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
    
    // 显示暂停菜单
    if (_gameMenus) {
        _gameMenus->showPauseMenu();
    }
    
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
    
    // 隐藏暂停菜单
    if (_gameMenus) {
        _gameMenus->hidePauseMenu();
    }
    
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
    
    // 显示游戏结束界面
    if (_gameMenus) {
        _gameMenus->showGameOver();
    }
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
    
    // 显示胜利界面
    if (_gameMenus) {
        _gameMenus->showVictory();
    }
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
    
    // 1-3通关后进入Boss层（stage=0表示Boss层）
    if (nextLevel == 1 && nextStage > 3)
    {
        nextStage = 0; // Boss层
    }
    
    // 使用静态变量传递信息给新场景
    s_nextLevel = nextLevel;
    s_nextStage = nextStage;
    s_savedHP = savedHP;
    s_savedMP = savedMP;
    s_savedItems = _collectedItems;  // 保存已收集的道具
    
    GAME_LOG("Set static vars for next scene: Level %d-%d, HP: %d, MP: %d, Items: %d", 
             nextLevel, nextStage, savedHP, savedMP, (int)_collectedItems.size());
    
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
    if (_gameMenus) {
        _gameMenus->setPauseMenuVisible(false);
    }
    
    // 创建设置层
    _settingsLayer = SettingsLayer::create();
    _settingsLayer->setCloseCallback([this]() {
        // 清除设置层指针
        _settingsLayer = nullptr;
        // 恢复显示暂停菜单
        if (_gameMenus) {
            _gameMenus->setPauseMenuVisible(true);
        }
    });
    _uiLayer->addChild(_settingsLayer);
}

void GameScene::addItemToUI(const ItemDef* itemDef)
{
    if (!itemDef || !_gameHUD)
    {
        return;
    }
    
    // 记录收集的道具ID（用于关卡继承）
    _collectedItems.push_back(itemDef->id);
    
    _gameHUD->addItemIcon(itemDef);
}
