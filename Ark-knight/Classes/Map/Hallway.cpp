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
    
    // 计算走廊尺寸
    // 房间宽度 = 25 * 32 = 800px，中心距离 = 900px
    // 走廊长度 = 900 - 800 = 100px ≈ 3瓦片
    // 房间高度 = 17 * 32 = 544px，中心距离 = 900px  
    // 走廊长度 = 900 - 544 = 356px ≈ 11瓦片
    
    float tileSize = Constants::FLOOR_TILE_SIZE;
    
    if (_direction == Constants::DIR_UP || _direction == Constants::DIR_DOWN) {
        // 垂直走廊：宽度为门宽，长度为垂直间隔
        _tilesWidth = Constants::DOOR_WIDTH;
        // 计算垂直间隔瓦片数
        float gap = Constants::ROOM_CENTER_DIST - Constants::ROOM_TILES_H * tileSize;
        _tilesHeight = static_cast<int>(gap / tileSize);
    } else {
        // 水平走廊：长度为水平间隔，宽度为门宽
        // 计算水平间隔瓦片数
        float gap = Constants::ROOM_CENTER_DIST - Constants::ROOM_TILES_W * tileSize;
        _tilesWidth = static_cast<int>(gap / tileSize);
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
            // 只在最外层添加墙壁，中间全是地板
            bool isTopEdge = (h == _tilesHeight - 1);
            bool isBottomEdge = (h == 0);
            bool isLeftEdge = (w == 0);
            bool isRightEdge = (w == _tilesWidth - 1);
            bool isEdge = isTopEdge || isBottomEdge || isLeftEdge || isRightEdge;
            
            if (isEdge) {
                // 边缘是墙壁
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
