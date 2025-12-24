#ifndef __ROOM_H__
#define __ROOM_H__

#include "cocos2d.h"
#include "Core/Constants.h"
#include "Map/Barriers.h"

class Enemy;
class Player;

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
    
protected:
    void generateFloor(float x, float y);
    void generateWall(float x, float y, int zOrder);
    void generateDoor(float x, float y, int direction);
    
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
};

#endif // __ROOM_H__
