#include "GameScene.h"
#include "MainMenuScene.h"
#include "Entities/Player/Mage.h"
#include <algorithm>

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
    _uiLayer->addChild(_miniMap);
    
    // 设置当前房间
    _currentRoom = _mapGenerator->getBeginRoom();
    
    GAME_LOG("Map system initialized with %d rooms", _mapGenerator->getRoomCount());
}

void GameScene::initCamera()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    
    // 创建正交相机（适合2D游戏）
    _camera = Camera::createOrthographic(visibleSize.width, visibleSize.height, 1, 1000);
    
    if (_camera && _player)
    {
        // 设置相机初始位置（跟随玩家）
        Vec3 playerPos3D(_player->getPositionX(), _player->getPositionY(), 0);
        _camera->setPosition3D(Vec3(playerPos3D.x, playerPos3D.y, 100));
        _camera->lookAt(playerPos3D, Vec3(0, 1, 0));
        
        // 将相机添加到游戏层
        _gameLayer->addChild(_camera);
        
        // 不要设置整个游戏层的CameraMask，让子节点继承默认相机
        // 这样就不会出现一片黑的问题
        _camera->setCameraFlag(CameraFlag::USER1);
        
        // 设置地图生成器和玩家使用自定义相机
        if (_mapGenerator) {
            _mapGenerator->setCameraMask((unsigned short)CameraFlag::USER1);
        }
        if (_player) {
            _player->setCameraMask((unsigned short)CameraFlag::USER1);
        }
        
        GAME_LOG("Camera initialized at player position (%.1f, %.1f)", 
                 _player->getPositionX(), _player->getPositionY());
    }
}

void GameScene::updateCamera(float dt)
{
    if (!_camera || !_player || !_mapGenerator) return;
    
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
    
    // 获取屏幕尺寸的一半（相机可见范围）
    auto visibleSize = Director::getInstance()->getVisibleSize();
    float halfWidth = visibleSize.width / 2.0f;
    float halfHeight = visibleSize.height / 2.0f;
    
    // 限制相机位置，使其不超出地图边界
    float camX = playerPos.x;
    float camY = playerPos.y;
    
    // 限制X轴
    if (maxX - minX > visibleSize.width) {
        // 地图宽度大于屏幕，限制相机不显示地图外区域
        camX = std::max(minX + halfWidth, camX);
        camX = std::min(maxX - halfWidth, camX);
    } else {
        // 地图宽度小于屏幕，相机居中显示整个地图
        camX = (minX + maxX) / 2.0f;
    }
    
    // 限制Y轴
    if (maxY - minY > visibleSize.height) {
        // 地图高度大于屏幕，限制相机不显示地图外区域
        camY = std::max(minY + halfHeight, camY);
        camY = std::min(maxY - halfHeight, camY);
    } else {
        // 地图高度小于屏幕，相机居中显示整个地图
        camY = (minY + maxY) / 2.0f;
    }
    
    // 平滑跟随（使用线性插值）
    Vec3 currentPos = _camera->getPosition3D();
    float smoothFactor = 0.1f;  // 平滑系数，越小越平滑但响应越慢
    
    Vec3 targetPos(camX, camY, 100);
    Vec3 newPos = currentPos + (targetPos - currentPos) * smoothFactor;
    
    _camera->setPosition3D(newPos);
    _camera->lookAt(Vec3(newPos.x, newPos.y, 0), Vec3(0, 1, 0));
}

void GameScene::createPlayer()
{
    // 创建法师角色
    _player = Mage::create();
    
    if (_player == nullptr) {
        GAME_LOG_ERROR("Failed to create Mage!");
        return;
    }
    
    // 设置玩家初始位置
    _player->setPosition(SCREEN_CENTER);
    
    // 创建玩家精灵（临时使用纯色方块）
    auto drawNode = DrawNode::create();
    drawNode->drawSolidRect(Vec2(-15, -15), Vec2(15, 15), Color4F::BLUE);
    drawNode->setGlobalZOrder(Constants::ZOrder::ENTITY);
    _player->addChild(drawNode);
    
    // 设置玩家的全局Z顺序
    _player->setGlobalZOrder(Constants::ZOrder::ENTITY);
    
    _gameLayer->addChild(_player);
    
    GAME_LOG("Player created at position (%.1f, %.1f)", _player->getPositionX(), _player->getPositionY());
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
        
        GAME_LOG("Enemy %d created at position (%.1f, %.1f)", i, randomX, randomY);
    }
}

void GameScene::createHUD()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();
    
    // HP标签
    _hpLabel = Label::createWithSystemFont("HP: 100/100", "Arial", 24);
    _hpLabel->setPosition(Vec2(origin.x + 100, origin.y + visibleSize.height - 30));
    _hpLabel->setTextColor(Color4B::GREEN);
    _uiLayer->addChild(_hpLabel);
    
    // MP标签
    _mpLabel = Label::createWithSystemFont("MP: 100/100", "Arial", 24);
    _mpLabel->setPosition(Vec2(origin.x + 100, origin.y + visibleSize.height - 60));
    _mpLabel->setTextColor(Color4B::BLUE);
    _uiLayer->addChild(_mpLabel);
    
    // Debug信息
    _debugLabel = Label::createWithSystemFont("", "Arial", 18);
    _debugLabel->setPosition(Vec2(origin.x + visibleSize.width - 150, origin.y + visibleSize.height - 30));
    _debugLabel->setTextColor(Color4B::YELLOW);
    _uiLayer->addChild(_debugLabel);
    
    // 操作提示
    auto hintLabel = Label::createWithSystemFont(
        "Controls:\nWASD - Move\nJ - Attack\nK - Skill\nSPACE - Dash\nESC - Pause",
        "Arial", 18);
    hintLabel->setPosition(Vec2(origin.x + 120, origin.y + 100));
    hintLabel->setTextColor(Color4B::WHITE);
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
    if (_mapGenerator == nullptr || _player == nullptr) return;
    
    Vec2 pos = _player->getPosition();
    
    // 首先检查玩家是否在走廊中
    Hallway* currentHallway = _mapGenerator->getPlayerHallway(_player);
    
    if (currentHallway != nullptr) {
        // 在走廊中，使用走廊的边界检测
        float dummyX = 0, dummyY = 0;
        currentHallway->checkPlayerPosition(_player, dummyX, dummyY);
        return;
    }
    
    // 检测玩家当前所在房间
    Room* newRoom = _mapGenerator->updatePlayerRoom(_player);
    
    if (newRoom != nullptr && newRoom != _currentRoom)
    {
        // 玩家进入了新房间
        _currentRoom = newRoom;
        
        // 更新小地图
        if (_miniMap)
        {
            _miniMap->updateCurrentRoom(_currentRoom);
        }
        
        GAME_LOG("Player entered room (%d, %d), type: %d", 
                 _currentRoom->getGridX(), 
                 _currentRoom->getGridY(),
                 static_cast<int>(_currentRoom->getRoomType()));
    }
    
    // 在房间内，使用房间的边界检测
    if (_currentRoom != nullptr)
    {
        Rect walkable = _currentRoom->getWalkableArea();
        Vec2 center = _currentRoom->getCenter();
        
        float doorHalfWidth = Constants::DOOR_WIDTH * Constants::FLOOR_TILE_SIZE / 2.0f;
        bool modified = false;
        
        // 检查边界，但允许在门口区域通过
        if (pos.x < walkable.getMinX()) {
            // 左边界：如果有门且在门范围内，允许通过；否则阻挡
            if (_currentRoom->hasDoor(Constants::DIR_LEFT) && 
                std::abs(pos.y - center.y) <= doorHalfWidth) {
                // 在门口区域，允许通过
            } else {
                pos.x = walkable.getMinX();
                modified = true;
            }
        }
        else if (pos.x > walkable.getMaxX()) {
            // 右边界
            if (_currentRoom->hasDoor(Constants::DIR_RIGHT) && 
                std::abs(pos.y - center.y) <= doorHalfWidth) {
                // 在门口区域，允许通过
            } else {
                pos.x = walkable.getMaxX();
                modified = true;
            }
        }
        
        if (pos.y < walkable.getMinY()) {
            // 下边界
            if (_currentRoom->hasDoor(Constants::DIR_DOWN) && 
                std::abs(pos.x - center.x) <= doorHalfWidth) {
                // 在门口区域，允许通过
            } else {
                pos.y = walkable.getMinY();
                modified = true;
            }
        }
        else if (pos.y > walkable.getMaxY()) {
            // 上边界
            if (_currentRoom->hasDoor(Constants::DIR_UP) && 
                std::abs(pos.x - center.x) <= doorHalfWidth) {
                // 在门口区域，允许通过
            } else {
                pos.y = walkable.getMaxY();
                modified = true;
            }
        }
        
        if (modified) {
            _player->setPosition(pos);
        }
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
    if (_player != nullptr)
    {
        // 更新HP
        char hpText[64];
        sprintf(hpText, "HP: %d/%d", _player->getHP(), _player->getMaxHP());
        _hpLabel->setString(hpText);
        
        // 更新MP
        char mpText[64];
        sprintf(mpText, "MP: %d/%d", _player->getMP(), _player->getMaxMP());
        _mpLabel->setString(mpText);
        
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
    
    // 显示暂停菜单
    auto pauseLabel = Label::createWithSystemFont("PAUSED\nPress ESC to resume\nQ to quit", 
                                                   "Arial", 48);
    pauseLabel->setPosition(SCREEN_CENTER);
    pauseLabel->setTextColor(Color4B::WHITE);
    pauseLabel->setName("pauseLabel");
    _uiLayer->addChild(pauseLabel, 1000);
    
    // 添加半透明遮罩
    auto mask = LayerColor::create(Color4B(0, 0, 0, 150));
    mask->setName("pauseMask");
    _uiLayer->addChild(mask, 999);
    
    GAME_LOG("Game paused");
}

void GameScene::resumeGame()
{
    _isPaused = false;
    
    // 移除暂停菜单
    _uiLayer->removeChildByName("pauseLabel");
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
    _uiLayer->addChild(mask, 999);
    
    // 显示游戏结束文字
    auto gameOverLabel = Label::createWithSystemFont("GAME OVER", "Arial", 64);
    gameOverLabel->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y + 50));
    gameOverLabel->setTextColor(Color4B::RED);
    gameOverLabel->setName("gameOverLabel");
    _uiLayer->addChild(gameOverLabel, 1000);
    
    // 显示重新开始提示
    auto hintLabel = Label::createWithSystemFont("Press R to Restart\nPress Q to Quit", "Arial", 32);
    hintLabel->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y - 50));
    hintLabel->setTextColor(Color4B::WHITE);
    hintLabel->setName("gameOverHint");
    _uiLayer->addChild(hintLabel, 1000);
    
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