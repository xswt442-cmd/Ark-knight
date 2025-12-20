#ifndef __ROOM_H__
#define __ROOM_H__

#include "cocos2d.h"
#include "Core/Constants.h"

class Enemy;
class Player;

/**
 * Room - 游戏房间类
 * 包含地板、墙壁、门、敌人等元素
 */
class Room : public cocos2d::Node {
public:
    static Room* create();
    
    virtual bool init() override;
    virtual void update(float delta) override;
    
    // 创建房间地图（地板、墙壁、门）
    void createMap();
    
    // 设置/获取房间中心坐标
    void setCenter(float x, float y);
    cocos2d::Vec2 getCenter() const { return cocos2d::Vec2(_centerX, _centerY); }
    
    // 设置/获取房间在矩阵中的位置
    void setGridPosition(int x, int y) { _gridX = x; _gridY = y; }
    int getGridX() const { return _gridX; }
    int getGridY() const { return _gridY; }
    
    // 设置/获取房间类型
    void setRoomType(Constants::RoomType type) { _roomType = type; }
    Constants::RoomType getRoomType() const { return _roomType; }
    
    // 门连接方向（上右下左）
    void setDoorOpen(int direction, bool open);
    bool hasDoor(int direction) const { return _doorDirections[direction]; }
    
    // 静态方法：创建并连接两个房间
    static bool connectRooms(Room* fromRoom, Room* toRoom, int direction);
    
    // 敌人管理
    void addEnemy(Enemy* enemy);
    void createEnemies(int count);
    void removeDeadEnemies();
    bool allEnemiesKilled() const;
    cocos2d::Vector<Enemy*>& getEnemies() { return _enemies; }
    
    // 门的开关控制
    void openDoors();
    void closeDoors();
    bool areDoorsOpen() const { return _doorsOpen; }
    
    // 玩家位置检测
    bool isPlayerInRoom(Player* player) const;
    
    // 获取房间边界（用于碰撞检测）
    cocos2d::Rect getRoomBounds() const;
    cocos2d::Rect getWalkableArea() const;  // 可行走区域（不包含墙壁）
    
    // 移动整个房间（用于视角跟随）
    void moveBy(float dx, float dy);
    
    // 玩家访问状态
    void setVisited(bool visited) { _visited = visited; }
    bool isVisited() const { return _visited; }
    
    // 获取门的位置
    cocos2d::Vec2 getDoorPosition(int direction) const;
    
protected:
    // 生成地板
    void generateFloor(float x, float y);
    
    // 生成墙壁
    void generateWall(float x, float y, int zOrder, bool hasShadow);
    
    // 生成门
    void generateDoor(float x, float y, int direction);
    
private:
    // 房间中心坐标（世界坐标）
    float _centerX;
    float _centerY;
    
    // 房间在地图矩阵中的位置
    int _gridX;
    int _gridY;
    
    // 房间尺寸（地板块数）
    int _tilesWidth;
    int _tilesHeight;
    
    // 房间边界顶点（世界坐标）
    float _leftX, _topY;      // 左上角
    float _rightX, _bottomY;  // 右下角
    
    // 房间类型
    Constants::RoomType _roomType;
    
    // 四个方向的门连接状态
    bool _doorDirections[4];  // UP, RIGHT, DOWN, LEFT
    
    // 门是否打开
    bool _doorsOpen;
    
    // 玩家是否访问过
    bool _visited;
    
    // 精灵容器
    cocos2d::Vector<cocos2d::Sprite*> _floors;
    cocos2d::Vector<cocos2d::Sprite*> _walls;
    cocos2d::Vector<cocos2d::Sprite*> _doorsOpen_sprites;
    cocos2d::Vector<cocos2d::Sprite*> _doorsClosed_sprites;
    
    // 敌人容器
    cocos2d::Vector<Enemy*> _enemies;
};

#endif // __ROOM_H__
