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
    startButton->setTitleText("Start Game");
    startButton->setTitleFontSize(32);
    startButton->setPosition(Vec2(centerX, startY));
    startButton->addClickEventListener(CC_CALLBACK_1(MainMenuScene::onStartGame, this));
    _uiLayer->addChild(startButton);
    
    // 选择角色按钮
    auto selectButton = Button::create();
    selectButton->setTitleText("Select Character");
    selectButton->setTitleFontSize(32);
    selectButton->setPosition(Vec2(centerX, startY - spacing));
    selectButton->addClickEventListener(CC_CALLBACK_1(MainMenuScene::onSelectCharacter, this));
    _uiLayer->addChild(selectButton);
    
    // 设置按钮
    auto settingsButton = Button::create();
    settingsButton->setTitleText("Settings");
    settingsButton->setTitleFontSize(32);
    settingsButton->setPosition(Vec2(centerX, startY - spacing * 2));
    settingsButton->addClickEventListener(CC_CALLBACK_1(MainMenuScene::onSettings, this));
    _uiLayer->addChild(settingsButton);
    
    // 退出按钮
    auto exitButton = Button::create();
    exitButton->setTitleText("Exit");
    exitButton->setTitleFontSize(32);
    exitButton->setPosition(Vec2(centerX, startY - spacing * 3));
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

void MainMenuScene::onSelectCharacter(Ref* sender)
{
    GAME_LOG("Select character clicked");
    
    // 移除之前的提示（如果有）
    this->removeChildByName("hintLabel");
    
    // TODO: 实现角色选择界面
    auto label = Label::createWithSystemFont("Character Selection - Coming Soon!", "Arial", 24);
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
    
    // 移除之前的提示（如果有）
    this->removeChildByName("hintLabel");
    
    // TODO: 实现设置界面
    auto label = Label::createWithSystemFont("Settings - Coming Soon!", "Arial", 24);
    label->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y + 100));
    label->setTextColor(Color4B::YELLOW);
    label->setName("hintLabel");
    this->addChild(label, 100);

    auto delay = DelayTime::create(2.0f);
    auto remove = RemoveSelf::create();
    label->runAction(Sequence::create(delay, remove, nullptr));
}

void MainMenuScene::onExit(Ref* sender)
{
    GAME_LOG("Exit clicked");
    
    // 退出游戏
    Director::getInstance()->end();
}
