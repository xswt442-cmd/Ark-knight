#ifndef __BOSS_FLOOR_H__
#define __BOSS_FLOOR_H__

#include "cocos2d.h"
#include "Room.h"
#include "Hallway.h"
#include "Core/Constants.h"

/**
 * BossFloor - Boss层地图生成器
 * 
 * 负责生成Boss层的所有内容，包括：
 * - 起始房间、Boss房间、以及新增的三阶段房间
 * - 房间之间的走廊连接
 * - Boss房间的火焰地板装饰
 */
class BossFloor : public cocos2d::Node {
public:
    // ==================== 可调整的参数 ====================
    
    /** 火焰地板数量 */
    static constexpr int FIRE_TILE_COUNT = 50;
    
    /** Boss中心区域排除半径（瓦片数），该区域内不生成火焰 */
    static constexpr int FIRE_EXCLUSION_RADIUS = 6;
    
    /** 火焰距离房间边缘的最小距离（瓦片数） */
    static constexpr int FIRE_EDGE_MARGIN = 3;
    
    // ==================== 公共接口 ====================
    
    static BossFloor* create();
    virtual bool init() override;
    
    /**
     * 生成Boss层地图
     * 
     * @param roomMatrix 房间矩阵引用，用于存储生成的房间
     * @param hallways 走廊列表引用，用于存储生成的走廊
     * @param beginRoom 输出参数：起始房间指针
     * @param endRoom 输出参数：Boss房间指针
     * @param roomCount 输出参数：房间数量
     */
    void generate(
        Room* (&roomMatrix)[Constants::MAP_GRID_SIZE][Constants::MAP_GRID_SIZE],
        std::vector<Hallway*>& hallways,
        Room*& beginRoom,
        Room*& endRoom,
        int& roomCount
    );
    
    /** 获取起始房间 */
    Room* getStartRoom() const { return _startRoom; }
    
    /** 获取Boss房间 */
    Room* getBossRoom() const { return _bossRoom; }

    /** 获取三阶段房间 */
    Room* getPhase3Room() const { return _phase3Room; }
    
private:
    // ==================== 内部方法 ====================
    
    /** 创建起始房间 */
    Room* createStartRoom(int gridX, int gridY);
    
    /** 创建Boss房间 */
    Room* createBossRoom(int gridX, int gridY, float startRoomRightEdge);

    /** 创建三阶段房间 (Boss房间的一半大小) */
    Room* createPhase3Room(int gridX, int gridY, float bossRoomRightEdge);
    
    /** 生成火焰地板装饰 */
    void generateFireTiles();
    
    /** 生成连接走廊 */
    void generateHallway(std::vector<Hallway*>& hallways);
    
    // ==================== 成员变量 ====================
    
    Room* _startRoom;
    Room* _bossRoom;
    Room* _phase3Room; // 新增：三阶段房间
    cocos2d::Vector<cocos2d::Sprite*> _fireFloors;
};

#endif // __BOSS_FLOOR_H__
