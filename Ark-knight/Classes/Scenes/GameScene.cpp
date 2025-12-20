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
    updateMapSystem(dt);  // 更新地图系统
    updateEnemies(dt);
    updateHUD(dt);
    checkCollisions();
}

void GameScene::updateMapSystem(float dt)
{
    if (_mapGenerator == nullptr || _player == nullptr) return;
    
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
    
    // 限制玩家在当前房间内
    if (_currentRoom != nullptr)
    {
        Rect walkable = _currentRoom->getWalkableArea();
        Vec2 pos = _player->getPosition();
        
        pos.x = std::max(walkable.getMinX(), std::min(walkable.getMaxX(), pos.x));
        pos.y = std::max(walkable.getMinY(), std::min(walkable.getMaxY(), pos.y));
        
        _player->setPosition(pos);
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