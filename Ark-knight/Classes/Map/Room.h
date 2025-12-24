#ifndef __ROOM_H__
#define __ROOM_H__

#include "cocos2d.h"
#include "Core/Constants.h"
#include "Map/Barriers.h"

class Enemy;
class Player;

/**
 * 普通战斗房间的地形布局类型
 */
enum class TerrainLayout {
    NONE = 0,           // 无特殊地形
    FIVE_BOXES,         // 5堆木箱（左上、左下、右上、右下、中间各3x3）
    NINE_BOXES,         // 9堆木箱（在FIVE_BOXES基础上加左中、上中、右中、下中）
    UPDOWN_SPIKES,      // 矩形围城-上下地刺（左右木箱，上下地刺，四角木箱）
    LEFTRIGHT_SPIKES,   // 矩形围城-左右地刺（上下木箱，左右地刺，四角木箱）
    ALL_SPIKES,         // 矩形围城-一圈地刺（四周地刺，四角也是地刺）
    UPDOWN_BOXES,       // 上下木箱（上下各一排木箱）
    LEFTRIGHT_BOXES,    // 左右木箱（左右各一排木箱）
    CENTER_PILLAR,      // 中心石柱（中间2x2石柱）
    FOUR_PILLARS,       // 4堆石柱（四个角各2x2石柱）
    RANDOM_MESS         // 乱七八糟（随机放置10个石柱+10个木箱）
};

/**
 * Room - 游戏房间类
 */
class Room : public cocos2d::Node {
public:
    static Room* create();
    
    virtual bool init() override;
    virtual void update(float delta) override;
    
    void createMap();
    
    void setCenter(float x, float y);
    cocos2d::Vec2 getCenter() const { return cocos2d::Vec2(_centerX, _centerY); }
    
    void setGridPosition(int x, int y) { _gridX = x; _gridY = y; }
    int getGridX() const { return _gridX; }
    int getGridY() const { return _gridY; }
    int getTilesWidth() const { return _tilesWidth; }
    int getTilesHeight() const { return _tilesHeight; }
    
    void setRoomType(Constants::RoomType type) { _roomType = type; }
    Constants::RoomType getRoomType() const { return _roomType; }
    
    void setDoorOpen(int direction, bool open);
    bool hasDoor(int direction) const;
    
    void createEnemies(int count);
    bool allEnemiesKilled() const;
    cocos2d::Vector<Enemy*>& getEnemies() { return _enemies; }

    // 地刺管理
    void addSpikeAtPosition(const cocos2d::Vec2& pos);
    void addSpikeAtTile(int tileX, int tileY);
    const cocos2d::Vector<Spike*>& getSpikes() const { return _spikes; }
    
    // Box和Pillar管理
    void addBoxAtPosition(const cocos2d::Vec2& pos, Box::BoxType type = Box::BoxType::NORMAL);
    void addBoxAtTile(int tileX, int tileY, Box::BoxType type = Box::BoxType::NORMAL);
    void addPillarAtPosition(const cocos2d::Vec2& pos, Pillar::PillarType type = Pillar::PillarType::CLEAR);
    void addPillarAtTile(int tileX, int tileY, Pillar::PillarType type = Pillar::PillarType::CLEAR);
    const cocos2d::Vector<Barrier*>& getBarriers() const { return _barriers; }
    
    void openDoors();
    void closeDoors();
    bool areDoorsOpen() const { return _doorsOpen; }
    
    bool isPlayerInRoom(Player* player) const;
    cocos2d::Rect getWalkableArea() const;
    void moveBy(float dx, float dy);
    
    // 检查玩家位置并修改速度（参考学长实现）
    bool checkPlayerPosition(Player* player, float& speedX, float& speedY);
    
    void setVisited(bool visited) { _visited = visited; }
    bool isVisited() const { return _visited; }
    
    // 敌人生成标记
    void setEnemiesSpawned(bool spawned) { _enemiesSpawned = spawned; }
    bool isEnemiesSpawned() const { return _enemiesSpawned; }
    
    // 地形布局
    void applyTerrainLayout(TerrainLayout layout);
    
    // 宝箱管理
    void createChest();
    cocos2d::Sprite* getChest() const { return _chest; }
    bool isChestOpened() const { return _chestOpened; }
    void openChest();
    bool canInteractWithChest(Player* player) const;
    
    // 传送门管理
    void createPortal();
    cocos2d::Sprite* getPortal() const { return _portal; }
    bool canInteractWithPortal(Player* player) const;
    
protected:
    void generateFloor(float x, float y);
    void generateWall(float x, float y, int zOrder);
    void generateDoor(float x, float y, int direction);
    
    // 地形布局辅助方法
    void layoutFiveBoxes();
    void layoutNineBoxes();
    void layoutUpDownSpikes();
    void layoutLeftRightSpikes();
    void layoutAllSpikes();
    void layoutUpDownBoxes();
    void layoutLeftRightBoxes();
    void layoutCenterPillar();
    void layoutFourPillars();
    void layoutRandomMess();
    
    // 辅助方法：添加一个3x3的木箱堆（单堆材质统一）
    void addBoxCluster3x3(int centerTileX, int centerTileY);
    // 辅助方法：添加一个2x2的石柱堆（单堆材质统一）
    void addPillarCluster2x2(int centerTileX, int centerTileY);
    
private:
    float _centerX;
    float _centerY;
    int _gridX;
    int _gridY;
    int _tilesWidth;
    int _tilesHeight;
    float _leftX, _topY;
    float _rightX, _bottomY;
    
    Constants::RoomType _roomType;
    bool _doorDirections[4];
    bool _doorsOpen;
    bool _visited;
    bool _enemiesSpawned;  // 是否已生成敌人
    int _floorTextureIndex;  // 随机选择的地板纹理索引(1-5)
    
    cocos2d::Vector<cocos2d::Sprite*> _floors;
    cocos2d::Vector<cocos2d::Sprite*> _walls;
    cocos2d::Vector<cocos2d::Sprite*> _doorsOpenSprites;
    cocos2d::Vector<cocos2d::Sprite*> _doorsClosedSprites;
    cocos2d::Vector<Enemy*> _enemies;
    cocos2d::Vector<Spike*> _spikes;
    cocos2d::Vector<Barrier*> _barriers;  // 所有障碍物(Box和Pillar)
    cocos2d::Sprite* _chest;  // 奖励房间的宝箱
    bool _chestOpened;  // 宝箱是否已打开
    cocos2d::Sprite* _portal;  // 传送门主体
    cocos2d::Sprite* _portalLighting;  // 传送门闪电特效
};

#endif // __ROOM_H__
