#ifndef __MAIN_MENU_SCENE_H__
#define __MAIN_MENU_SCENE_H__

#include "cocos2d.h"
#include "Core/Constants.h"
#include "Core/GameMacros.h"
#include "UI/SettingsLayer.h"

USING_NS_CC;

// 主菜单场景 - 游戏启动后的第一个场景
class MainMenuScene : public Scene {
public:
    static Scene* createScene();
    virtual bool init() override;
    CREATE_FUNC(MainMenuScene);
    
private:
    // UI创建方法
    void createUI();
    void createBackground();
    void createButtons();
    
    // 按钮回调函数
    void onStartGame(Ref* sender);           // 开始游戏
    void onStartBossLevel(Ref* sender);      // 开始boss关卡
    
    // 选择角色回调
    void onSelectCharacter(Ref* sender);
    
    // 设置回调
    void onSettings(Ref* sender);
    
    // 退出游戏回调
    void onExit(Ref* sender);
    
    // 显示设置菜单
    void showSettings();
    
private:
    Layer* _uiLayer;
};

#endif // __MAIN_MENU_SCENE_H__
