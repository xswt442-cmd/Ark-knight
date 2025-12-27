#include "MainMenuScene.h"
#include "GameScene.h"
#include "ui/CocosGUI.h"

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
    
    GAME_LOG("MainMenuScene initialized");
    
    return true;
}

void MainMenuScene::createBackground()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();
    
    // 创建背景色
    auto background = LayerColor::create(Color4B(50, 50, 50, 255));
    this->addChild(background, -1);
    
    // 游戏标题
    auto titleLabel = Label::createWithSystemFont("Ark Knights", "Arial", 64);
    titleLabel->setPosition(Vec2(origin.x + visibleSize.width / 2,
                                 origin.y + visibleSize.height * 0.75f));
    titleLabel->setTextColor(Color4B::WHITE);
    this->addChild(titleLabel, 1);
}

void MainMenuScene::createUI()
{
    _uiLayer = Layer::create();
    this->addChild(_uiLayer, Constants::ZOrder::UI);
    
    createButtons();
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
    startButton->setTitleText(u8"开始游戏");
    startButton->setTitleFontName("fonts/msyh.ttf");
    startButton->setTitleFontSize(32);
    startButton->setPosition(Vec2(centerX, startY));
    startButton->addClickEventListener(CC_CALLBACK_1(MainMenuScene::onStartGame, this));
    _uiLayer->addChild(startButton);
    
    // 选择角色按钮
    auto selectButton = Button::create();
    selectButton->setTitleText(u8"选择角色");
    selectButton->setTitleFontName("fonts/msyh.ttf");
    selectButton->setTitleFontSize(32);
    selectButton->setPosition(Vec2(centerX, startY - spacing));
    selectButton->addClickEventListener(CC_CALLBACK_1(MainMenuScene::onSelectCharacter, this));
    _uiLayer->addChild(selectButton);
    
    // 跳转1-3按钮（测试boss）
    auto bossButton = Button::create();
    bossButton->setTitleText(u8"1-3 Boss关");
    bossButton->setTitleFontName("fonts/msyh.ttf");
    bossButton->setTitleFontSize(32);
    bossButton->setPosition(Vec2(centerX, startY - spacing * 2));
    bossButton->addClickEventListener(CC_CALLBACK_1(MainMenuScene::onStartBossLevel, this));
    _uiLayer->addChild(bossButton);
    
    // 设置按钮
    auto settingsButton = Button::create();
    settingsButton->setTitleText(u8"设置");
    settingsButton->setTitleFontName("fonts/msyh.ttf");
    settingsButton->setTitleFontSize(32);
    settingsButton->setPosition(Vec2(centerX, startY - spacing * 3));
    settingsButton->addClickEventListener(CC_CALLBACK_1(MainMenuScene::onSettings, this));
    _uiLayer->addChild(settingsButton);
    
    // 退出按钮
    auto exitButton = Button::create();
    exitButton->setTitleText(u8"退出游戏");
    exitButton->setTitleFontName("fonts/msyh.ttf");
    exitButton->setTitleFontSize(32);
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
    GAME_LOG("Start boss level (1-3) clicked");
    
    // 设置为1-3关卡
    GameScene::s_nextLevel = 1;
    GameScene::s_nextStage = 3;
    
    // 切换到游戏场景
    auto gameScene = GameScene::createScene();
    Director::getInstance()->replaceScene(TransitionFade::create(1.0f, gameScene));
}

void MainMenuScene::onSelectCharacter(Ref* sender)
{
    GAME_LOG("Select character clicked");
    
    // 移除之前的提示（如果有）
    this->removeChildByName("hintLabel");
    
    // TODO: 实现角色选择界面
    auto label = Label::createWithSystemFont(u8"角色选择功能 - 敬请期待！", "Arial", 24);
    label->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y + 100));
    label->setTextColor(Color4B::YELLOW);
    label->setName("hintLabel");
    this->addChild(label, 100);
    
    // 2秒后移除
    auto delay = DelayTime::create(2.0f);
    auto remove = RemoveSelf::create();
    label->runAction(Sequence::create(delay, remove, nullptr));
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
