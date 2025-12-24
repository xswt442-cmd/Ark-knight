#ifndef __MAIN_MENU_SCENE_H__
#define __MAIN_MENU_SCENE_H__

#include "cocos2d.h"
#include "Core/Constants.h"
#include "Core/GameMacros.h"

USING_NS_CC;

/**
 * 主菜单场景
 * 游戏启动后的第一个场景
 * 包含：开始游戏、选择角色、设置、退出等功能
 */
class MainMenuScene : public Scene {
public:
    static Scene* createScene();
    
    virtual bool init() override;
    
    CREATE_FUNC(MainMenuScene);
    
private:
    /**
     * 创建UI元素
     */
    void createUI();
    
    /**
     * 创建背景
     */
    void createBackground();
    
    /**
     * 创建按钮
     */
    void createButtons();
    
    // ==================== 回调函数 ====================
    /**
     * 开始游戏回调
     */
    void onStartGame(Ref* sender);
    
    /**
     * 选择角色回调
     */
    void onSelectCharacter(Ref* sender);
    
    /**
     * 设置回调
     */
    void onSettings(Ref* sender);
    
    /**
     * 退出游戏回调
     */
    void onExit(Ref* sender);
    
    /**
     * 显示设置菜单
     */
    void showSettings();
    
    /**
     * 隐藏设置菜单
     */
    void hideSettings();
    
private:
    Layer* _uiLayer;
};

#endif // __MAIN_MENU_SCENE_H__
