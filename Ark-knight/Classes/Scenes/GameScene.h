#ifndef __GAME_SCENE_H__
#define __GAME_SCENE_H__

#include "cocos2d.h"
#include "Core/Constants.h"
#include "Core/GameMacros.h"
#include "Entities/Player/Player.h"
#include "Entities/Enemy/Enemy.h"

USING_NS_CC;

/**
 * 游戏主场景
 * 核心战斗场景，包含：
 * - 游戏逻辑层
 * - 地图层
 * - UI层
 */
class GameScene : public Scene {
public:
    static Scene* createScene();
    
    virtual bool init() override;
    virtual void update(float dt) override;
    
    CREATE_FUNC(GameScene);
    
    /**
     * 获取玩家对象
     */
    Player* getPlayer() const { return _player; }
    
private:
    /**
     * 初始化层级
     */
    void initLayers();
    
    /**
     * 创建玩家
     */
    void createPlayer();
    
    /**
     * 创建测试敌人
     */
    void createTestEnemies();
    
    /**
     * 创建HUD
     */
    void createHUD();
    
    /**
     * 更新玩家
     */
    void updatePlayer(float dt);
    
    /**
     * 更新敌人
     */
    void updateEnemies(float dt);
    
    /**
     * 更新HUD
     */
    void updateHUD(float dt);
    
    /**
     * 检测碰撞
     */
    void checkCollisions();
    
    /**
     * 暂停游戏
     */
    void pauseGame();
    
    /**
     * 继续游戏
     */
    void resumeGame();
    
    /**
     * 显示游戏结束界面
     */
    void showGameOver();
    
    // ==================== 按键回调 ====================
    void setupKeyboardListener();
    
private:
    // 图层
    Layer* _gameLayer;        // 游戏逻辑层
    Layer* _uiLayer;          // UI层
    
    // 游戏对象
    Player* _player;
    Vector<Enemy*> _enemies;
    
    // HUD元素
    Label* _hpLabel;
    Label* _mpLabel;
    Label* _debugLabel;
    
    // 状态
    bool _isPaused;
    bool _isGameOver;
};

#endif // __GAME_SCENE_H__
