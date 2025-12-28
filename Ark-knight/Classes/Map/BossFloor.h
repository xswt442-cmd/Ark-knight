#ifndef __BOSS_FLOOR_H__
#define __BOSS_FLOOR_H__

#include "cocos2d.h"
#include "Room.h"
#include "Hallway.h"
#include "Core/Constants.h"

// Boss层地图生成器 - 负责生成Boss层的所有内容
class BossFloor : public cocos2d::Node {
public:
    // 可调整的参数
    static constexpr int FIRE_TILE_COUNT = 50;           // 火焰地板数量
    static constexpr int FIRE_EXCLUSION_RADIUS = 6;      // Boss中心区域排除半径
    static constexpr int FIRE_EDGE_MARGIN = 3;           // 火焰距离房间边缘的最小距离
    
    static BossFloor* create();
    virtual bool init() override;
    
    // 生成Boss层地图，包括起始房间、Boss房间和三阶段房间
    void generate(
        Room* (&roomMatrix)[Constants::MAP_GRID_SIZE][Constants::MAP_GRID_SIZE],
        std::vector<Hallway*>& hallways,
        Room*& beginRoom,
        Room*& endRoom,
        int& roomCount
    );
    
    Room* getStartRoom() const { return _startRoom; }
    Room* getBossRoom() const { return _bossRoom; }
    Room* getPhase3Room() const { return _phase3Room; }
    
private:
    // 内部房间创建方法
    Room* createStartRoom(int gridX, int gridY);
    Room* createBossRoom(int gridX, int gridY, float startRoomRightEdge);
    Room* createPhase3Room(int gridX, int gridY, float bossRoomRightEdge);  // Boss房间的一半大小
    
    // 地形装饰和连接
    void generateFireTiles();                             // 生成火焰地板装饰
    void generateHallway(std::vector<Hallway*>& hallways);
    
    // 成员变量
    
    Room* _startRoom;
    Room* _bossRoom;
    Room* _phase3Room; // 三阶段房间
    cocos2d::Vector<cocos2d::Sprite*> _fireFloors;
};

#endif // __BOSS_FLOOR_H__
