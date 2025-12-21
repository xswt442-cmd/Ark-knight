#include "Room.h"
#include "Entities/Enemy/Enemy.h"
#include "Entities/Player/Player.h"

USING_NS_CC;

// 方向偏移数组
static const int DIR_DX[] = {0, 1, 0, -1};  // UP, RIGHT, DOWN, LEFT
static const int DIR_DY[] = {1, 0, -1, 0};

Room* Room::create() {
    Room* room = new (std::nothrow) Room();
    if (room && room->init()) {
        room->autorelease();
        return room;
    }
    CC_SAFE_DELETE(room);
    return nullptr;
}

bool Room::init() {
    if (!Node::init()) {
        return false;
    }
    
    _centerX = 0.0f;
    _centerY = 0.0f;
    _gridX = 0;
    _gridY = 0;
    _tilesWidth = Constants::ROOM_TILES_W;
    _tilesHeight = Constants::ROOM_TILES_H;
    _leftX = 0.0f;
    _topY = 0.0f;
    _rightX = 0.0f;
    _bottomY = 0.0f;
    _roomType = Constants::RoomType::NORMAL;
    _doorsOpen = true;
    _visited = false;
    
    for (int i = 0; i < Constants::DIR_COUNT; i++) {
        _doorDirections[i] = false;
    }
    
    this->scheduleUpdate();
    return true;
}

void Room::update(float delta) {
    if (allEnemiesKilled() && !_doorsOpen) {
        openDoors();
    }
}

void Room::setCenter(float x, float y) {
    _centerX = x;
    _centerY = y;
    
    float tileSize = Constants::FLOOR_TILE_SIZE;
    float halfWidth = tileSize * (_tilesWidth / 2.0f);
    float halfHeight = tileSize * (_tilesHeight / 2.0f);
    
    // 房间内部可行走区域（排除墙壁）
    // 偶数瓦片：墙壁在最外一圈，边界紧贴墙壁内侧
    // halfWidth = 26*32/2 = 416, 墙壁边缘在 ±416，可行走边界在 ±(416-32)
    _leftX = _centerX - halfWidth + tileSize;   // 左墙右边缘
    _rightX = _centerX + halfWidth - tileSize;  // 右墙左边缘
    _topY = _centerY + halfHeight - tileSize;   // 上墙下边缘
    _bottomY = _centerY - halfHeight + tileSize; // 下墙上边缘
}

void Room::createMap() {
    // 根据房间类型调整大小
    switch (_roomType) {
        case Constants::RoomType::BOSS:
            _tilesWidth = Constants::ROOM_TILES_W + 4;
            _tilesHeight = Constants::ROOM_TILES_H + 4;
            break;
        case Constants::RoomType::WEAPON:
        case Constants::RoomType::PROP:
        case Constants::RoomType::END:
            _tilesWidth = Constants::ROOM_TILES_W - 4;
            _tilesHeight = Constants::ROOM_TILES_H - 4;
            break;
        default:
            break;
    }
    
    setCenter(_centerX, _centerY);
    
    float tileSize = Constants::FLOOR_TILE_SIZE;
    float startX = _centerX - tileSize * (_tilesWidth / 2.0f);
    float startY = _centerY + tileSize * (_tilesHeight / 2.0f);
    
    float curX = startX;
    float curY = startY;
    
    int doorWidth = Constants::DOOR_WIDTH;
    
    for (int h = _tilesHeight - 1; h >= 0; h--) {
        for (int w = 0; w < _tilesWidth; w++) {
            bool isEdge = (h == 0 || h == _tilesHeight - 1 || w == 0 || w == _tilesWidth - 1);
            
            if (isEdge) {
                bool isDoor = false;
                int doorDir = -1;
                int doorStart, doorEnd;
                
                // 上方门
                if (h == _tilesHeight - 1 && _doorDirections[Constants::DIR_UP]) {
                    doorStart = _tilesWidth / 2 - doorWidth / 2;
                    doorEnd = doorStart + doorWidth - 1;
                    if (w >= doorStart && w <= doorEnd) {
                        isDoor = true;
                        doorDir = Constants::DIR_UP;
                    }
                }
                // 下方门
                if (h == 0 && _doorDirections[Constants::DIR_DOWN]) {
                    doorStart = _tilesWidth / 2 - doorWidth / 2;
                    doorEnd = doorStart + doorWidth - 1;
                    if (w >= doorStart && w <= doorEnd) {
                        isDoor = true;
                        doorDir = Constants::DIR_DOWN;
                    }
                }
                // 左方门
                if (w == 0 && _doorDirections[Constants::DIR_LEFT]) {
                    doorStart = _tilesHeight / 2 - doorWidth / 2;
                    doorEnd = doorStart + doorWidth - 1;
                    if (h >= doorStart && h <= doorEnd) {
                        isDoor = true;
                        doorDir = Constants::DIR_LEFT;
                    }
                }
                // 右方门
                if (w == _tilesWidth - 1 && _doorDirections[Constants::DIR_RIGHT]) {
                    doorStart = _tilesHeight / 2 - doorWidth / 2;
                    doorEnd = doorStart + doorWidth - 1;
                    if (h >= doorStart && h <= doorEnd) {
                        isDoor = true;
                        doorDir = Constants::DIR_RIGHT;
                    }
                }
                
                if (isDoor) {
                    generateDoor(curX, curY, doorDir);
                } else {
                    int zOrder = (h == _tilesHeight - 1) ? Constants::ZOrder::WALL_BELOW : Constants::ZOrder::WALL_ABOVE;
                    generateWall(curX, curY, zOrder);
                }
            } else {
                generateFloor(curX, curY);
            }
            
            curX += tileSize;
        }
        curX = startX;
        curY -= tileSize;
    }
}

void Room::generateFloor(float x, float y) {
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

void Room::generateWall(float x, float y, int zOrder) {
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

void Room::generateDoor(float x, float y, int direction) {
    float tileSize = Constants::FLOOR_TILE_SIZE;
    
    auto doorOpen = Sprite::create("res/door_open.png");
    if (!doorOpen) {
        doorOpen = Sprite::create();
        doorOpen->setTextureRect(Rect(0, 0, tileSize, tileSize));
        doorOpen->setColor(Color3B(50, 50, 70));
    }
    doorOpen->setPosition(Vec2(x, y));
    doorOpen->setGlobalZOrder(Constants::ZOrder::DOOR);
    this->addChild(doorOpen, Constants::ZOrder::DOOR);
    _doorsOpenSprites.pushBack(doorOpen);
    
    auto doorClosed = Sprite::create("res/door_closed.png");
    if (!doorClosed) {
        doorClosed = Sprite::create();
        doorClosed->setTextureRect(Rect(0, 0, tileSize, tileSize));
        doorClosed->setColor(Color3B(120, 60, 30));
    }
    doorClosed->setPosition(Vec2(x, y));
    doorClosed->setGlobalZOrder(Constants::ZOrder::WALL_ABOVE);
    doorClosed->setVisible(false);
    this->addChild(doorClosed, Constants::ZOrder::WALL_ABOVE);
    _doorsClosedSprites.pushBack(doorClosed);
}

void Room::setDoorOpen(int direction, bool open) {
    if (direction >= 0 && direction < Constants::DIR_COUNT) {
        _doorDirections[direction] = open;
    }
}

bool Room::hasDoor(int direction) const {
    if (direction >= 0 && direction < Constants::DIR_COUNT) {
        return _doorDirections[direction];
    }
    return false;
}

void Room::createEnemies(int count) {
    for (int i = 0; i < count; i++) {
        float randX = _centerX + (rand() % 200 - 100);
        float randY = _centerY + (rand() % 150 - 75);
        
        auto enemy = Enemy::create();
        if (enemy) {
            enemy->setPosition(Vec2(randX, randY));
            _enemies.pushBack(enemy);
            this->addChild(enemy);
        }
    }
}

bool Room::allEnemiesKilled() const {
    if (_roomType == Constants::RoomType::BEGIN) {
        return true;
    }
    
    for (auto enemy : _enemies) {
        if (enemy->getHP() > 0) {
            return false;
        }
    }
    return true;
}

void Room::openDoors() {
    _doorsOpen = true;
    for (auto& sprite : _doorsOpenSprites) {
        sprite->setVisible(true);
    }
    for (auto& sprite : _doorsClosedSprites) {
        sprite->setVisible(false);
    }
}

void Room::closeDoors() {
    _doorsOpen = false;
    for (auto& sprite : _doorsOpenSprites) {
        sprite->setVisible(false);
    }
    for (auto& sprite : _doorsClosedSprites) {
        sprite->setVisible(true);
    }
}

bool Room::isPlayerInRoom(Player* player) const {
    if (!player) return false;
    
    Vec2 pos = player->getPosition();
    return (pos.x >= _leftX && pos.x <= _rightX &&
            pos.y >= _bottomY && pos.y <= _topY);
}

Rect Room::getWalkableArea() const {
    // 直接返回已计算好的边界
    float width = _rightX - _leftX;
    float height = _topY - _bottomY;
    return Rect(_leftX, _bottomY, width, height);
}

void Room::moveBy(float dx, float dy) {
    _centerX += dx;
    _centerY += dy;
    _leftX += dx;
    _rightX += dx;
    _topY += dy;
    _bottomY += dy;
    
    for (auto child : this->getChildren()) {
        Vec2 pos = child->getPosition();
        child->setPosition(pos.x + dx, pos.y + dy);
    }
}
