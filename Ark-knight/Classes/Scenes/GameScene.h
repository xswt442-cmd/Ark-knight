#ifndef __GAME_SCENE_H__
#define __GAME_SCENE_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "Core/Constants.h"
#include "Core/GameMacros.h"
#include "Entities/Player/Player.h"
#include "Entities/Enemy/Enemy.h"
#include "Entities/Enemy/Ayao.h"
#include "Map/MapGenerator.h"
#include "Map/Room.h"
#include "UI/MiniMap.h"
#include "UI/SettingsLayer.h"

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
    
    /**
     * 新增：由外部注册新生成的敌人（例如 Enemy::die 生成的 KongKaZi）
     */
    void addEnemy(Enemy* enemy);
    
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
     * 在指定房间生成敌人
     * @param room 目标房间
     */
    void spawnEnemiesInRoom(Room* room);
    
    /**
     * 初始化地图系统
     */
    void initMapSystem();
    
    /**
     * 初始化相机系统
     */
    void initCamera();
    
    /**
     * 更新相机位置（跟随玩家）
     */
    void updateCamera(float dt);
    
    /**
     * 更新地图和房间
     */
    void updateMapSystem(float dt);
    
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
     * 显示设置菜单
     */
    void showSettings();
    
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
    
    // 地图系统
    MapGenerator* _mapGenerator;
    MiniMap* _miniMap;
    Room* _currentRoom;
    
    // 相机系统
    Camera* _camera;
    
    // HUD元素
    cocos2d::ui::LoadingBar* _hpBar;     // 血条
    cocos2d::ui::LoadingBar* _mpBar;     // 蓝条
    Sprite* _hpIcon;                      // 爱心图标
    Sprite* _mpIcon;                      // 闪电图标
    Label* _hpLabel;                      // 血量数值
    Label* _mpLabel;                      // 蓝量数值
    Label* _debugLabel;
    Label* _skillLabel;
    
    // 技能图标系统
    Sprite* _skillIcon;                   // 角色技能图标
    ProgressTimer* _skillCDProgress;      // 角色技能CD进度
    Sprite* _skillCDMask;                 // 角色技能CD变暗遮罩
    
    // 治疗技能图标系统
    Sprite* _healIcon;                    // 治疗技能图标
    ProgressTimer* _healCDProgress;       // 治疗技能CD进度
    Sprite* _healCDMask;                  // 治疗技能CD变暗遮罩
    
    // 状态
    bool _isPaused;
    bool _isGameOver;
};

#endif // __GAME_SCENE_H__
