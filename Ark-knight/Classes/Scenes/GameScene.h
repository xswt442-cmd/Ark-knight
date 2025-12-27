#ifndef __GAME_SCENE_H__
#define __GAME_SCENE_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "Core/Constants.h"
#include "Core/GameMacros.h"
#include "Entities/Player/Player.h"
#include "Entities/Enemy/Enemy.h"
#include "Entities/Objects/Item.h"
#include "Map/MapGenerator.h"
#include "Map/Room.h"
#include "UI/MiniMap.h"
#include "UI/SettingsLayer.h"
#include "UI/GameHUD.h"
#include "UI/GameMenus.h"
#include "Map/Barriers.h"
#include "Entities/Objects/Item.h"

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
    
    // 静态变量：用于场景切换时传递关卡信息和血蓝量
    static int s_nextLevel;
    static int s_nextStage;
    static int s_savedHP;
    static int s_savedMP;
    static std::vector<std::string> s_savedItems;  // 保存的道具ID列表
    
    /**
     * 获取玩家对象
     */
    Player* getPlayer() const { return _player; }
    
    /**
     * 新增：由外部注册新生成的敌人（例如 Enemy::die 生成的 KongKaZi）
     */
    void addEnemy(Enemy* enemy);
    
    /**
     * 添加道具到UI显示
     * @param itemDef 道具定义
     */
    void addItemToUI(const ItemDef* itemDef);
    
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
     * 创建菜单系统
     */
    void createMenus();
    
    /**
     * 更新玩家
     */
    void updatePlayer(float dt);
    
    /**
     * 更新敌人
     */
    void updateEnemies(float dt);
    void updateSpikes(float dt);
    
    /**
     * 更新交互系统
     */
    void updateInteraction(float dt);
    
    /**
     * 更新HUD
     */
    void updateHUD(float dt);
    
    /**
     * 检测碰撞
     */
    void checkCollisions();
    
    /**
     * 检测并解决实体与障碍物的碰撞
     */
    void checkBarrierCollisions();
    
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
    
    /**
     * 显示胜利界面
     */
    void showVictory();
    
    /**
     * 切换到下一关
     */
    void goToNextLevel();
    
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
    
    // HUD和菜单系统
    GameHUD* _gameHUD;
    GameMenus* _gameMenus;
    
    // 状态
    bool _isPaused;
    bool _isGameOver;
    bool _keyE;  // E键交互状态
    
    // 关卡系统
    int _currentLevel;     // 当前大关（1-3）
    int _currentStage;     // 当前小关（1-5）
    int _savedHP;          // 保存的血量
    int _savedMP;          // 保存的蓝量
    std::vector<std::string> _collectedItems;  // 已收集的道具ID列表
};

#endif // __GAME_SCENE_H__
