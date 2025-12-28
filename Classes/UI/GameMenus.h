#ifndef __GAME_MENUS_H__
#define __GAME_MENUS_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "Core/Constants.h"
#include "Core/GameMacros.h"

USING_NS_CC;

class Player;
class Enemy;

// 游戏菜单管理类 - 负责管理游戏中的各种菜单界面：暂停菜单、游戏结束界面、胜利界面
class GameMenus : public Node {
public:
    static GameMenus* create();
    virtual bool init() override;
    
    // 回调类型定义
    using ResumeCallback = std::function<void()>;
    using SettingsCallback = std::function<void()>;
    using RestartCallback = std::function<void()>;
    using MainMenuCallback = std::function<void()>;
    
    // 设置回调函数
    void setResumeCallback(const ResumeCallback& callback) { _resumeCallback = callback; }
    void setSettingsCallback(const SettingsCallback& callback) { _settingsCallback = callback; }
    void setRestartCallback(const RestartCallback& callback) { _restartCallback = callback; }
    void setMainMenuCallback(const MainMenuCallback& callback) { _mainMenuCallback = callback; }
    
    // 显示暂停菜单
    void showPauseMenu();
    
    // 隐藏暂停菜单
    void hidePauseMenu();
    
    // 设置暂停菜单可见性（用于设置界面切换）
    void setPauseMenuVisible(bool visible);
    
    // 显示游戏结束界面
    void showGameOver();
    
    // 显示胜利界面
    void showVictory();
    
    // 是否有菜单显示中
    bool isMenuVisible() const { return _isMenuVisible; }
    
private:
    void createPauseMenuElements();
    void createGameOverElements();
    void createVictoryElements();
    void clearMenu();
    
private:
    // 遮罩层
    LayerColor* _maskLayer;
    
    // 暂停菜单元素
    Label* _pauseTitle;
    ui::Button* _resumeBtn;
    ui::Button* _settingsBtn;
    ui::Button* _mainMenuBtn;
    ui::Button* _exitBtn;
    
    // 游戏结束元素
    Label* _gameOverLabel;
    Label* _gameOverHint;
    
    // 胜利元素
    Label* _victoryLabel;
    Label* _victoryHint;
    
    // 回调函数
    ResumeCallback _resumeCallback;
    SettingsCallback _settingsCallback;
    RestartCallback _restartCallback;
    MainMenuCallback _mainMenuCallback;
    
    // 状态
    bool _isMenuVisible;
};

#endif // __GAME_MENUS_H__
