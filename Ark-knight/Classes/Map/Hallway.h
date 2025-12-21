#ifndef __HALLWAY_H__
#define __HALLWAY_H__

#include "cocos2d.h"
#include "Core/Constants.h"

/**
 * Hallway - 连接房间的走廊
 */
class Hallway : public cocos2d::Node {
public:
    static Hallway* create(int direction);
    
    virtual bool init() override;
    bool initWithDirection(int direction);
    
    void createMap();
    void setCenter(float x, float y);
    
    int getDirection() const { return _direction; }
    cocos2d::Vec2 getCenter() const { return cocos2d::Vec2(_centerX, _centerY); }
    
private:
    void generateFloor(float x, float y);
    
    float _centerX;
    float _centerY;
    int _direction;  // UP, RIGHT, DOWN, LEFT
    int _tilesWidth;
    int _tilesHeight;
    
    cocos2d::Vector<cocos2d::Sprite*> _floors;
};

#endif // __HALLWAY_H__
