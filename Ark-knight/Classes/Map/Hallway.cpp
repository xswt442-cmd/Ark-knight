#include "Hallway.h"

USING_NS_CC;

Hallway* Hallway::create(int direction) {
    Hallway* hallway = new (std::nothrow) Hallway();
    if (hallway && hallway->initWithDirection(direction)) {
        hallway->autorelease();
        return hallway;
    }
    CC_SAFE_DELETE(hallway);
    return nullptr;
}

bool Hallway::init() {
    if (!Node::init()) {
        return false;
    }
    return true;
}

bool Hallway::initWithDirection(int direction) {
    if (!init()) {
        return false;
    }
    
    _direction = direction;
    _centerX = 0.0f;
    _centerY = 0.0f;
    
    // 根据方向设置走廊尺寸
    if (_direction == Constants::DIR_UP || _direction == Constants::DIR_DOWN) {
        // 垂直走廊
        _tilesWidth = Constants::DOOR_WIDTH;
        _tilesHeight = 8;  // 走廊长度
    } else {
        // 水平走廊
        _tilesWidth = 8;
        _tilesHeight = Constants::DOOR_WIDTH;
    }
    
    return true;
}

void Hallway::setCenter(float x, float y) {
    _centerX = x;
    _centerY = y;
}

void Hallway::createMap() {
    float tileSize = Constants::FLOOR_TILE_SIZE;
    float startX = _centerX - tileSize * (_tilesWidth / 2.0f);
    float startY = _centerY + tileSize * (_tilesHeight / 2.0f);
    
    float curX = startX;
    float curY = startY;
    
    for (int h = _tilesHeight - 1; h >= 0; h--) {
        for (int w = 0; w < _tilesWidth; w++) {
            bool isEdge = (h == 0 || h == _tilesHeight - 1 || w == 0 || w == _tilesWidth - 1);
            
            if (isEdge) {
                // 边缘是墙壁
                bool isTopEdge = (h == _tilesHeight - 1);
                int zOrder = isTopEdge ? Constants::ZOrder::WALL_BELOW : Constants::ZOrder::WALL_ABOVE;
                generateWall(curX, curY, zOrder);
            } else {
                // 内部是地板
                generateFloor(curX, curY);
            }
            
            curX += tileSize;
        }
        curX = startX;
        curY -= tileSize;
    }
}

void Hallway::generateFloor(float x, float y) {
    auto floor = Sprite::create("res/floor.png");
    if (!floor) {
        floor = Sprite::create();
        floor->setTextureRect(Rect(0, 0, Constants::FLOOR_TILE_SIZE, Constants::FLOOR_TILE_SIZE));
        floor->setColor(Color3B(60, 60, 80));
    }
    
    floor->setPosition(Vec2(x, y));
    floor->setGlobalZOrder(Constants::ZOrder::FLOOR);
    this->addChild(floor, Constants::ZOrder::FLOOR);
    _floors.pushBack(floor);
}

void Hallway::generateWall(float x, float y, int zOrder) {
    auto wall = Sprite::create("res/wall.png");
    if (!wall) {
        wall = Sprite::create();
        wall->setTextureRect(Rect(0, 0, Constants::FLOOR_TILE_SIZE, Constants::FLOOR_TILE_SIZE));
        wall->setColor(Color3B(80, 80, 100));
    }
    
    wall->setPosition(Vec2(x, y));
    wall->setGlobalZOrder(zOrder);
    this->addChild(wall, zOrder);
    _walls.pushBack(wall);
}
