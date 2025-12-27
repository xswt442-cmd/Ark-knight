#include "MainMenuScene.h"
#include "GameScene.h"
#include "UI/CharacterSelectLayer.h"
#include "ui/CocosGUI.h"
#include "audio/include/AudioEngine.h"

USING_NS_CC;
using namespace ui;

Scene* MainMenuScene::createScene()
{
    return MainMenuScene::create();
}

bool MainMenuScene::init()
{
    if (!Scene::init())
    {
        return false;
    }
    
    createBackground();
    createUI();
    
    // 播放主菜单背景音乐
    AudioEngine::stopAll();
    AudioEngine::play2d("Music/Menu.mp3", true);
    
    GAME_LOG("MainMenuScene initialized");
    
    return true;
}

void MainMenuScene::createBackground()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();
    
    // 创建背景图片
    auto background = Sprite::create("Scene.png");
    if (background)
    {
        // 缩放以适应屏幕
        background->setPosition(Vec2(origin.x + visibleSize.width / 2,
                                     origin.y + visibleSize.height / 2));
        float scaleX = visibleSize.width / background->getContentSize().width;
        float scaleY = visibleSize.height / background->getContentSize().height;
        background->setScale(std::max(scaleX, scaleY));
        this->addChild(background, -1);
    }
    else
    {
        // 备用：如果图片加载失败，使用灰色背景
        auto bgColor = LayerColor::create(Color4B(50, 50, 50, 255));
        this->addChild(bgColor, -1);
    }
    
}

void MainMenuScene::createUI()
{
    _uiLayer = Layer::create();
    this->addChild(_uiLayer, Constants::ZOrder::UI);
    
    createButtons();
}

// 辅助函数：创建带黑边深黄色粗体样式的按钮
static void styleMenuButton(ui::Button* button, const std::string& text)
{
    button->setTitleText(text);
    button->setTitleFontName("fonts/msyh.ttf");
    button->setTitleFontSize(32);
    // 深黄色
    button->setTitleColor(Color3B(220, 180, 0));
    // 获取标题渲染器添加黑边描边
    auto titleRenderer = button->getTitleRenderer();
    if (titleRenderer)
    {
        titleRenderer->enableOutline(Color4B::BLACK, 2);
        titleRenderer->enableBold();
    }
}

void MainMenuScene::createButtons()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();
    
    float centerX = origin.x + visibleSize.width / 2;
    float startY = origin.y + visibleSize.height * 0.5f;
    float spacing = 80.0f;
    
    // 开始游戏按钮
    auto startButton = Button::create();
    styleMenuButton(startButton, u8"开始游戏");
    startButton->setPosition(Vec2(centerX, startY));
    startButton->addClickEventListener(CC_CALLBACK_1(MainMenuScene::onStartGame, this));
    _uiLayer->addChild(startButton);
    
    // 选择角色按钮
    auto selectButton = Button::create();
    styleMenuButton(selectButton, u8"选择角色");
    selectButton->setPosition(Vec2(centerX, startY - spacing));
    selectButton->addClickEventListener(CC_CALLBACK_1(MainMenuScene::onSelectCharacter, this));
    _uiLayer->addChild(selectButton);
    
    // 直接挑战Boss按钮
    auto bossButton = Button::create();
    styleMenuButton(bossButton, u8"直接挑战Boss");
    bossButton->setPosition(Vec2(centerX, startY - spacing * 2));
    bossButton->addClickEventListener(CC_CALLBACK_1(MainMenuScene::onStartBossLevel, this));
    _uiLayer->addChild(bossButton);
    
    // 设置按钮
    auto settingsButton = Button::create();
    styleMenuButton(settingsButton, u8"设置");
    settingsButton->setPosition(Vec2(centerX, startY - spacing * 3));
    settingsButton->addClickEventListener(CC_CALLBACK_1(MainMenuScene::onSettings, this));
    _uiLayer->addChild(settingsButton);
    
    // 退出按钮
    auto exitButton = Button::create();
    styleMenuButton(exitButton, u8"退出游戏");
    exitButton->setPosition(Vec2(centerX, startY - spacing * 4));
    exitButton->addClickEventListener(CC_CALLBACK_1(MainMenuScene::onExit, this));
    _uiLayer->addChild(exitButton);
}

void MainMenuScene::onStartGame(Ref* sender)
{
    GAME_LOG("Start game clicked");
    
    // 切换到游戏场景
    auto gameScene = GameScene::createScene();
    Director::getInstance()->replaceScene(TransitionFade::create(1.0f, gameScene));
}

void MainMenuScene::onStartBossLevel(Ref* sender)
{
    GAME_LOG("Start boss floor clicked");
    
    // 设置为Boss层（stage=0表示Boss层）
    GameScene::s_nextLevel = 1;
    GameScene::s_nextStage = 0;
    
    // 切换到游戏场景
    auto gameScene = GameScene::createScene();
    Director::getInstance()->replaceScene(TransitionFade::create(1.0f, gameScene));
}

void MainMenuScene::onSelectCharacter(Ref* sender)
{
    GAME_LOG("Select character clicked");
    
    // 移除之前的提示（如果有）
    this->removeChildByName("hintLabel");
    
    // 创建角色选择层
    auto selectLayer = CharacterSelectLayer::create();
    selectLayer->setCloseCallback([this]() {
        GAME_LOG("Character selection closed");
    });
    this->addChild(selectLayer, 100);
}

void MainMenuScene::onSettings(Ref* sender)
{
    GAME_LOG("Settings clicked");
    showSettings();
}

void MainMenuScene::onExit(Ref* sender)
{
    GAME_LOG("Exit clicked");
    
    // 退出游戏
    Director::getInstance()->end();
}

void MainMenuScene::showSettings()
{
    GAME_LOG("Opening settings menu");
    
    // 隐藏主菜单按钮
    for (auto child : _uiLayer->getChildren())
    {
        child->setVisible(false);
    }
    
    // 创建设置层
    auto settingsLayer = SettingsLayer::create();
    settingsLayer->setCloseCallback([this]() {
        // 恢复显示主菜单按钮
        for (auto child : _uiLayer->getChildren())
        {
            child->setVisible(true);
        }
    });
    this->addChild(settingsLayer);
}
