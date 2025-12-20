#ifndef __MINI_MAP_H__
#define __MINI_MAP_H__

#include "cocos2d.h"
#include "Core/Constants.h"

class MapGenerator;
class Room;

/**
 * MiniRoom - 小地图上的单个房间显示
 */
class MiniRoom : public cocos2d::Node {
public:
    static MiniRoom* create();
    virtual bool init() override;
    
    // 设置房间类型颜色
    void setRoomColor(Constants::RoomType type);
    
    // 设置为当前房间（高亮显示）
    void setCurrent(bool isCurrent);
    
    // 设置已访问（灰色显示）
    void setVisited(bool visited);
    
    // 设置门的可见性
    void setDoorVisible(int direction, bool visible);
    
private:
    cocos2d::DrawNode* _background;
    cocos2d::DrawNode* _doors[4];  // 四个方向的门
    bool _isCurrent;
};

/**
 * MiniMap - 小地图UI组件
 * 显示在屏幕右上角，展示房间布局
 */
class MiniMap : public cocos2d::Node {
public:
    static MiniMap* create();
    virtual bool init() override;
    
    // 根据地图生成器初始化小地图
    void initFromMapGenerator(MapGenerator* generator);
    
    // 更新当前房间显示
    void updateCurrentRoom(Room* currentRoom);
    
    // 更新房间访问状态
    void updateRoomVisited(int gridX, int gridY);
    
    // 获取小地图房间
    MiniRoom* getMiniRoom(int x, int y);
    
private:
    // 5x5 小地图房间矩阵
    MiniRoom* _miniRooms[Constants::Map::MAP_SIZE][Constants::Map::MAP_SIZE];
    
    // 小地图配置
    float _roomSize;      // 每个房间的显示大小
    float _gap;           // 房间之间的间隙
    float _totalWidth;    // 小地图总宽度
    float _totalHeight;   // 小地图总高度
    
    // 当前高亮的房间
    Room* _currentRoom;
    int _currentGridX;
    int _currentGridY;
};

#endif // __MINI_MAP_H__
