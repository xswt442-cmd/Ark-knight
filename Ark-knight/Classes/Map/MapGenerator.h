#ifndef __MAP_GENERATOR_H__
#define __MAP_GENERATOR_H__

#include "cocos2d.h"
#include "Core/Constants.h"
#include "Room.h"
#include "Hallway.h"
#include <queue>
#include <vector>

/**
 * MapGenerator - 地图生成器
 * 使用BFS算法随机生成Roguelike风格的房间布局
 */
class MapGenerator : public cocos2d::Node {
public:
    static MapGenerator* create();
    
    virtual bool init() override;
    virtual void update(float delta) override;
    
    // 生成新的地图
    void generateMap();
    
    // 获取房间矩阵
    Room* getRoom(int x, int y);
    Room* getCurrentRoom() { return _currentRoom; }
    
    // 获取起始和结束房间
    Room* getBeginRoom() { return _beginRoom; }
    Room* getEndRoom() { return _endRoom; }
    
    // 设置当前房间
    void setCurrentRoom(Room* room);
    
    // 获取房间数量
    int getRoomCount() const { return _roomCount; }
    
    // 检查并更新玩家所在房间
    Room* updatePlayerRoom(class Player* player);
    
    // 设置关卡数
    void setLevelNumber(int level, int stage) { _levelNumber = (level - 1) * 5 + stage; }
    
    // 设置为Boss层模式（只生成起始房间+Boss房间）
    void setBossFloor(bool isBoss) { _isBossFloor = isBoss; }
    bool isBossFloor() const { return _isBossFloor; }
    
    // 移动所有房间（用于视角跟随）
    void moveAllRoomsBy(float dx, float dy);
    
    // 获取房间在世界坐标中的位置
    cocos2d::Vec2 getRoomWorldPosition(int gridX, int gridY);
    
    // 获取玩家所在的走廊
    Hallway* getPlayerHallway(class Player* player);
    
    // 获取所有房间（用于遍历检测）
    std::vector<Room*> getAllRooms() const;
    
    // 获取所有走廊
    const std::vector<Hallway*>& getAllHallways() const { return _hallways; }
    
    // 清理地图
    void clearMap();
    
private:
    // BFS随机生成房间
    void randomGenerate(int startX, int startY);
    
    // 从当前房间向外扩展
    void expandFromRoom(int x, int y, Room* curRoom, std::queue<Room*>& q);
    
    // 设置房间类型（起点、终点、Boss、道具等）
    void assignRoomTypes();
    
    // 连接所有相邻房间的门
    void connectAdjacentRooms();
    
    // 生成走廊
    void generateHallways();
    
    // 随机选择普通战斗房间地形布局（概率：空10%，其余各9%）
    TerrainLayout pickRandomTerrainLayout() const;
    
private:
    // 5x5 房间矩阵
    Room* _roomMatrix[Constants::MAP_GRID_SIZE][Constants::MAP_GRID_SIZE];
    
    // 走廊容器
    std::vector<Hallway*> _hallways;
    
    // 房间计数
    int _roomCount;
    
    // 特殊房间指针
    Room* _beginRoom;
    Room* _endRoom;
    Room* _currentRoom;
    
    // 当前关卡编号
    int _levelNumber;
    
    // 是否为Boss层
    bool _isBossFloor;
};

#endif // __MAP_GENERATOR_H__
