#include "GameMenus.h"
#include "Scenes/MainMenuScene.h"

GameMenus* GameMenus::create()
{
    GameMenus* ret = new (std::nothrow) GameMenus();
    if (ret && ret->init())
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool GameMenus::init()
{
    if (!Node::init())
    {
        return false;
    }
    
    _maskLayer = nullptr;
    _pauseTitle = nullptr;
    _resumeBtn = nullptr;
    _settingsBtn = nullptr;
    _mainMenuBtn = nullptr;
    _exitBtn = nullptr;
    _gameOverLabel = nullptr;
    _gameOverHint = nullptr;
    _victoryLabel = nullptr;
    _victoryHint = nullptr;
    _isMenuVisible = false;
    
    _resumeCallback = nullptr;
    _settingsCallback = nullptr;
    _restartCallback = nullptr;
    _mainMenuCallback = nullptr;
    
    return true;
}

void GameMenus::clearMenu()
{
    this->removeAllChildren();
    _maskLayer = nullptr;
    _pauseTitle = nullptr;
    _resumeBtn = nullptr;
    _settingsBtn = nullptr;
    _mainMenuBtn = nullptr;
    _exitBtn = nullptr;
    _gameOverLabel = nullptr;
    _gameOverHint = nullptr;
    _victoryLabel = nullptr;
    _victoryHint = nullptr;
    _isMenuVisible = false;
}

void GameMenus::showPauseMenu()
{
    clearMenu();
    _isMenuVisible = true;
    
    // 添加半透明遮罩
    _maskLayer = LayerColor::create(Color4B(0, 0, 0, 180));
    _maskLayer->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL);
    this->addChild(_maskLayer);
    
    // 标题
    _pauseTitle = Label::createWithTTF(u8"游戏已暂停", "fonts/msyh.ttf", 56);
    _pauseTitle->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y + 120));
    _pauseTitle->setTextColor(Color4B::WHITE);
    _pauseTitle->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 1);
    this->addChild(_pauseTitle);
    
    // 继续游戏按钮
    _resumeBtn = ui::Button::create();
    _resumeBtn->setTitleText(u8"继续游戏");
    _resumeBtn->setTitleFontName("fonts/msyh.ttf");
    _resumeBtn->setTitleFontSize(32);
    _resumeBtn->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y + 60));
    _resumeBtn->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 1);
    _resumeBtn->addClickEventListener([this](Ref* sender) {
        if (_resumeCallback) {
            _resumeCallback();
        }
    });
    this->addChild(_resumeBtn);
    
    // 设置按钮
    _settingsBtn = ui::Button::create();
    _settingsBtn->setTitleText(u8"设置");
    _settingsBtn->setTitleFontName("fonts/msyh.ttf");
    _settingsBtn->setTitleFontSize(32);
    _settingsBtn->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y - 10));
    _settingsBtn->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 1);
    _settingsBtn->addClickEventListener([this](Ref* sender) {
        if (_settingsCallback) {
            _settingsCallback();
        }
    });
    this->addChild(_settingsBtn);
    
    // 返回主菜单按钮
    _mainMenuBtn = ui::Button::create();
    _mainMenuBtn->setTitleText(u8"返回主菜单");
    _mainMenuBtn->setTitleFontName("fonts/msyh.ttf");
    _mainMenuBtn->setTitleFontSize(32);
    _mainMenuBtn->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y - 80));
    _mainMenuBtn->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 1);
    _mainMenuBtn->addClickEventListener([this](Ref* sender) {
        if (_mainMenuCallback) {
            _mainMenuCallback();
        } else {
            auto menuScene = MainMenuScene::createScene();
            Director::getInstance()->replaceScene(TransitionFade::create(0.5f, menuScene));
        }
    });
    this->addChild(_mainMenuBtn);
    
    // 退出游戏按钮
    _exitBtn = ui::Button::create();
    _exitBtn->setTitleText(u8"退出游戏");
    _exitBtn->setTitleFontName("fonts/msyh.ttf");
    _exitBtn->setTitleFontSize(32);
    _exitBtn->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y - 150));
    _exitBtn->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 1);
    _exitBtn->addClickEventListener([](Ref* sender) {
        Director::getInstance()->end();
    });
    this->addChild(_exitBtn);
    
    GAME_LOG("Pause menu shown");
}

void GameMenus::hidePauseMenu()
{
    clearMenu();
    GAME_LOG("Pause menu hidden");
}

void GameMenus::setPauseMenuVisible(bool visible)
{
    if (_pauseTitle) _pauseTitle->setVisible(visible);
    if (_resumeBtn) _resumeBtn->setVisible(visible);
    if (_settingsBtn) _settingsBtn->setVisible(visible);
    if (_mainMenuBtn) _mainMenuBtn->setVisible(visible);
    if (_exitBtn) _exitBtn->setVisible(visible);
}

void GameMenus::showGameOver()
{
    clearMenu();
    _isMenuVisible = true;
    
    GAME_LOG("Game Over!");
    
    // 添加半透明遮罩
    _maskLayer = LayerColor::create(Color4B(0, 0, 0, 180));
    _maskLayer->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL);
    this->addChild(_maskLayer);
    
    // 显示游戏结束图片
    auto gameOverSprite = Sprite::create("Enemy/GameOver.png");
    if (gameOverSprite)
    {
        gameOverSprite->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y + 50));
        gameOverSprite->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 1);
        this->addChild(gameOverSprite);
    }
    else
    {
        // 备用：如果图片加载失败，使用文字
        _gameOverLabel = Label::createWithTTF(u8"游戏结束", "fonts/msyh.ttf", 64);
        _gameOverLabel->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y + 50));
        _gameOverLabel->setTextColor(Color4B::RED);
        _gameOverLabel->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 1);
        this->addChild(_gameOverLabel);
    }
    
    // 显示重新开始提示
    _gameOverHint = Label::createWithTTF(u8"按 R 重新开始\n按 Q 退出到主菜单", "fonts/msyh.ttf", 32);
    _gameOverHint->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y - 100));
    _gameOverHint->setTextColor(Color4B::WHITE);
    _gameOverHint->setAlignment(TextHAlignment::CENTER);
    _gameOverHint->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 1);
    this->addChild(_gameOverHint);
    
    // 添加键盘监听
    auto listener = EventListenerKeyboard::create();
    listener->onKeyPressed = [this](EventKeyboard::KeyCode keyCode, Event* event) {
        if (keyCode == EventKeyboard::KeyCode::KEY_R)
        {
            if (_restartCallback) {
                _restartCallback();
            }
        }
        else if (keyCode == EventKeyboard::KeyCode::KEY_Q)
        {
            if (_mainMenuCallback) {
                _mainMenuCallback();
            } else {
                auto menuScene = MainMenuScene::createScene();
                Director::getInstance()->replaceScene(TransitionFade::create(0.5f, menuScene));
            }
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);
}

void GameMenus::showVictory()
{
    clearMenu();
    _isMenuVisible = true;
    
    GAME_LOG("Victory!");
    
    // 添加半透明遮罩
    _maskLayer = LayerColor::create(Color4B(0, 0, 0, 180));
    _maskLayer->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL);
    this->addChild(_maskLayer);
    
    // 显示胜利文字
    _victoryLabel = Label::createWithTTF(u8"通关成功！", "fonts/msyh.ttf", 64);
    _victoryLabel->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y + 50));
    _victoryLabel->setTextColor(Color4B::YELLOW);
    _victoryLabel->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 1);
    this->addChild(_victoryLabel);
    
    // 显示提示
    _victoryHint = Label::createWithTTF(u8"按 R 重新开始\n按 Q 退出到主菜单", "fonts/msyh.ttf", 32);
    _victoryHint->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y - 50));
    _victoryHint->setTextColor(Color4B::WHITE);
    _victoryHint->setAlignment(TextHAlignment::CENTER);
    _victoryHint->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 1);
    this->addChild(_victoryHint);
    
    // 添加键盘监听
    auto listener = EventListenerKeyboard::create();
    listener->onKeyPressed = [this](EventKeyboard::KeyCode keyCode, Event* event) {
        if (keyCode == EventKeyboard::KeyCode::KEY_R)
        {
            if (_restartCallback) {
                _restartCallback();
            }
        }
        else if (keyCode == EventKeyboard::KeyCode::KEY_Q)
        {
            if (_mainMenuCallback) {
                _mainMenuCallback();
            } else {
                auto menuScene = MainMenuScene::createScene();
                Director::getInstance()->replaceScene(TransitionFade::create(0.5f, menuScene));
            }
        }
    };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, this);
}
