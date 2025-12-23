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
    _enemiesSpawned = false;  // 初始化敌人生成标记
    _floorTextureIndex = (rand() % 5) + 1;  // 随机选择1-5号地板
    
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
    float playerHalfSize = 15.0f;  // 玩家半径约15像素
    
    // 房间内部可行走区域（排除墙壁并考虑玩家尺寸）
    // 边界要留出玩家半径的距离，防止卡墙
    _leftX = _centerX - halfWidth + tileSize + playerHalfSize;   // 左墙右边缘 + 玩家半径
    _rightX = _centerX + halfWidth - tileSize - playerHalfSize;  // 右墙左边缘 - 玩家半径
    _topY = _centerY + halfHeight - tileSize - playerHalfSize;   // 上墙下边缘 - 玩家半径
    _bottomY = _centerY - halfHeight + tileSize + playerHalfSize; // 下墙上边缘 + 玩家半径
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
    // 对于偶数瓦片，中心在两个瓦片之间
    // 第0列瓦片中心 = centerX - tileSize * (width/2 - 0.5)
    float startX = _centerX - tileSize * (_tilesWidth / 2.0f - 0.5f);
    float startY = _centerY + tileSize * (_tilesHeight / 2.0f - 0.5f);
    
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
    // 决定本房间使用哪个组：如果初始化的_floorTextureIndex是1~3，则视为组A(1-3)，否则为组B(4-5)
    int chosenIndex = _floorTextureIndex; // 作为回退值
    if (_floorTextureIndex >= 1 && _floorTextureIndex <= 3) {
        // 组 A: Floor1, Floor2, Floor3 -> 权重 75%,15%,10% (总和100)
        const int indices[3] = {1, 2, 3};
        const int weights[3] = {75, 15, 10};
        const int total = weights[0] + weights[1] + weights[2]; // 100
        int r = rand() % total;
        int acc = 0;
        for (int i = 0; i < 3; ++i) {
            acc += weights[i];
            if (r < acc) {
                chosenIndex = indices[i];
                break;
            }
        }
    } else {
        // 组 B: Floor4, Floor5 -> 权重 50%,50% (总和100)
        const int indices[2] = {4, 5};
        const int weights[2] = {50, 50};
        const int total = weights[0] + weights[1]; // 100
        int r = rand() % total;
        int acc = 0;
        for (int i = 0; i < 2; ++i) {
            acc += weights[i];
            if (r < acc) {
                chosenIndex = indices[i];
                break;
            }
        }
    }

    // 使用选定的纹理创建地板（与原逻辑一致，保留回退处理）
    std::string floorPath = "Map/Floor/Floor_000" + std::to_string(chosenIndex) + ".png";
    auto floor = Sprite::create(floorPath);
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
    auto wall = Sprite::create("Map/Wall/Wall_0001.png");
    if (!wall) {
        wall = Sprite::create();
        wall->setTextureRect(Rect(0, 0, Constants::FLOOR_TILE_SIZE, Constants::FLOOR_TILE_SIZE));
        wall->setColor(Color3B(80, 80, 100));
    }
    
    wall->setPosition(Vec2(x, y));
    wall->setGlobalZOrder(zOrder);
    wall->setTag(Constants::Tag::WALL);
    this->addChild(wall, zOrder);
    _walls.pushBack(wall);
}

void Room::generateDoor(float x, float y, int direction) {
    float tileSize = Constants::FLOOR_TILE_SIZE;
    
    auto doorOpen = Sprite::create("Map/Door/Door_open.png");
    if (!doorOpen) {
        doorOpen = Sprite::create();
        doorOpen->setTextureRect(Rect(0, 0, tileSize, tileSize));
        doorOpen->setColor(Color3B(50, 50, 70));
    }
    doorOpen->setPosition(Vec2(x, y));
    doorOpen->setGlobalZOrder(Constants::ZOrder::DOOR);
    this->addChild(doorOpen, Constants::ZOrder::DOOR);
    _doorsOpenSprites.pushBack(doorOpen);
    
    auto doorClosed = Sprite::create("Map/Door/Door_closed.png");
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
        // 移除墙的Tag，允许通行
        sprite->setTag(0);
    }
    
    GAME_LOG("Room doors opened");
}

void Room::closeDoors() {
    _doorsOpen = false;
    for (auto& sprite : _doorsOpenSprites) {
        sprite->setVisible(false);
    }
    for (auto& sprite : _doorsClosedSprites) {
        sprite->setVisible(true);
        // 设置为墙的Tag，阻止通行（包括玩家和弹幕）
        sprite->setTag(Constants::Tag::WALL);
    }
    
    GAME_LOG("Room doors closed");
}

bool Room::isPlayerInRoom(Player* player) const {
    if (!player) return false;
    
    // 直接使用玩家的本地坐标（相对于_gameLayer）
    // 房间边界也是相对于_gameLayer的，所以不需要坐标转换
    Vec2 pos = player->getPosition();
    
    // 检测是否在房间核心区域内
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

bool Room::checkPlayerPosition(Player* player, float& speedX, float& speedY) {
    if (!player) return false;
    
    // 直接使用玩家的本地坐标（相对于_gameLayer）
    // 房间边界也是相对于_gameLayer的，所以不需要坐标转换
    Vec2 pos = player->getPosition();
    float tileSize = Constants::FLOOR_TILE_SIZE;
    
    // 扩展检测范围（一格容差，用于检测是否在房间或门口附近）
    float extendedLeft = _leftX - tileSize;
    float extendedRight = _rightX + tileSize;
    float extendedTop = _topY + tileSize;
    float extendedBottom = _bottomY - tileSize;
    
    // 检查玩家是否在房间扩展范围内
    if (pos.x >= extendedLeft && pos.x <= extendedRight &&
        pos.y >= extendedBottom && pos.y <= extendedTop) {
        
        // 门口区域的半宽度
        float doorHalfWidth = Constants::DOOR_WIDTH * tileSize / 2.0f;
        
        // 检查是否所有敌人都被击杀
        bool canPassDoor = allEnemiesKilled();
        
        if (!canPassDoor) {
            // 敌人未全部击杀，不能通过任何门，限制在房间内
            if (speedX > 0 && pos.x >= _rightX) speedX = 0.0f;
            if (speedX < 0 && pos.x <= _leftX) speedX = 0.0f;
            if (speedY > 0 && pos.y >= _topY) speedY = 0.0f;
            if (speedY < 0 && pos.y <= _bottomY) speedY = 0.0f;
        }
        else {
            // 敌人已击杀，检查门口
            // 左右方向检测
            bool atVerticalDoorLevel = std::abs(pos.y - _centerY) <= doorHalfWidth;
            
            // 右边墙检测
            if (speedX > 0 && pos.x >= _rightX) {
                if (!_doorDirections[Constants::DIR_RIGHT] || !atVerticalDoorLevel) {
                    speedX = 0.0f;
                }
            }
            // 左边墙检测
            if (speedX < 0 && pos.x <= _leftX) {
                if (!_doorDirections[Constants::DIR_LEFT] || !atVerticalDoorLevel) {
                    speedX = 0.0f;
                }
            }
            
            // 上下方向检测
            bool atHorizontalDoorLevel = std::abs(pos.x - _centerX) <= doorHalfWidth;
            
            // 上边墙检测
            if (speedY > 0 && pos.y >= _topY) {
                if (!_doorDirections[Constants::DIR_UP] || !atHorizontalDoorLevel) {
                    speedY = 0.0f;
                }
            }
            // 下边墙检测
            if (speedY < 0 && pos.y <= _bottomY) {
                if (!_doorDirections[Constants::DIR_DOWN] || !atHorizontalDoorLevel) {
                    speedY = 0.0f;
                }
            }
        }
        
        return true;  // 玩家在房间范围内
    }
    
    return false;  // 玩家不在房间范围内
}
