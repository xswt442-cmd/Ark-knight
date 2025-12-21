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
    
    // 走廊只在房间之间的空隙区域
    // 房间中心距离 = 900px，房间宽度 = 800px，房间高度 = 544px
    // 空隙 = 900 - 800 = 100px (水平) 或 900 - 544 = 356px (垂直)
    
    float tileSize = Constants::FLOOR_TILE_SIZE;
    
    if (_direction == Constants::DIR_UP || _direction == Constants::DIR_DOWN) {
        // 垂直走廊
        _tilesWidth = Constants::DOOR_WIDTH;
        float gap = Constants::ROOM_CENTER_DIST - Constants::ROOM_TILES_H * tileSize;
        _tilesHeight = static_cast<int>(gap / tileSize);
    } else {
        // 水平走廊  
        float gap = Constants::ROOM_CENTER_DIST - Constants::ROOM_TILES_W * tileSize;
        _tilesWidth = static_cast<int>(gap / tileSize);
        _tilesHeight = Constants::DOOR_WIDTH;
    }
    
    log("Hallway created: direction=%d, size=(%d, %d)", direction, _tilesWidth, _tilesHeight);
    
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
    
    // 走廊只生成地板，不生成墙壁（墙壁由房间提供）
    for (int h = _tilesHeight - 1; h >= 0; h--) {
        for (int w = 0; w < _tilesWidth; w++) {
            // 所有位置都是地板
            generateFloor(curX, curY);
            curX += tileSize;
        }
        curX = startX;
        curY -= tileSize;
    }
    
    log("Hallway map created at (%.1f, %.1f), tiles: %d x %d", _centerX, _centerY, _tilesWidth, _tilesHeight);
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

Rect Hallway::getWalkableArea() const {
    float tileSize = Constants::FLOOR_TILE_SIZE;
    
    // 走廊全部都是可行走区域（没有内部墙壁）
    float startX = _centerX - tileSize * (_tilesWidth / 2.0f);
    float startY = _centerY + tileSize * (_tilesHeight / 2.0f);
    
    float minX = startX;
    float maxX = startX + tileSize * _tilesWidth;
    float maxY = startY;
    float minY = startY - tileSize * _tilesHeight;
    
    return Rect(minX, minY, maxX - minX, maxY - minY);
}

bool Hallway::isPlayerInHallway(Player* player) const {
    if (!player) return false;
    return getWalkableArea().containsPoint(player->getPosition());
}
