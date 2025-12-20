#include "Room.h"
#include "Entities/Enemy/Enemy.h"
#include "Entities/Player/Player.h"

USING_NS_CC;

using namespace Constants;

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
    
    // 初始化默认值
    _centerX = 0.0f;
    _centerY = 0.0f;
    _gridX = 0;
    _gridY = 0;
    _tilesWidth = Map::ROOM_TILES_WIDTH;
    _tilesHeight = Map::ROOM_TILES_HEIGHT;
    _leftX = 0.0f;
    _topY = 0.0f;
    _rightX = 0.0f;
    _bottomY = 0.0f;
    _roomType = RoomType::NORMAL;
    _doorsOpen = true;
    _visited = false;
    
    // 初始化门方向
    for (int i = 0; i < Direction::COUNT; i++) {
        _doorDirections[i] = false;
    }
    
    this->scheduleUpdate();
    return true;
}

void Room::update(float delta) {
    // 移除死亡敌人
    removeDeadEnemies();
    
    // 如果所有敌人都被杀死，打开门
    if (allEnemiesKilled() && !_doorsOpen) {
        openDoors();
    }
}

void Room::setCenter(float x, float y) {
    _centerX = x;
    _centerY = y;
    
    // 计算房间边界
    float halfWidth = Map::FLOOR_WIDTH * (_tilesWidth / 2);
    float halfHeight = Map::FLOOR_HEIGHT * (_tilesHeight / 2);
    
    _leftX = _centerX - halfWidth + Map::FLOOR_WIDTH;
    _rightX = _centerX + halfWidth - Map::FLOOR_WIDTH;
    _topY = _centerY + halfHeight - Map::FLOOR_HEIGHT;
    _bottomY = _centerY - halfHeight + Map::FLOOR_HEIGHT;
}

void Room::createMap() {
    // 根据房间类型调整大小
    switch (_roomType) {
        case RoomType::BOSS:
            _tilesWidth = Map::ROOM_TILES_WIDTH + 4;
            _tilesHeight = Map::ROOM_TILES_HEIGHT + 4;
            break;
        case RoomType::WEAPON:
        case RoomType::PROP:
        case RoomType::END:
            _tilesWidth = Map::ROOM_TILES_WIDTH - 4;
            _tilesHeight = Map::ROOM_TILES_HEIGHT - 4;
            break;
        default:
            _tilesWidth = Map::ROOM_TILES_WIDTH;
            _tilesHeight = Map::ROOM_TILES_HEIGHT;
            break;
    }
    
    // 重新计算边界
    setCenter(_centerX, _centerY);
    
    // 计算起始位置（左上角）
    float startX = _centerX - Map::FLOOR_WIDTH * (_tilesWidth / 2);
    float startY = _centerY + Map::FLOOR_HEIGHT * (_tilesHeight / 2);
    
    float curX = startX;
    float curY = startY;
    
    // 遍历生成地图元素
    for (int h = _tilesHeight - 1; h >= 0; h--) {
        for (int w = 0; w < _tilesWidth; w++) {
            bool isEdge = (h == 0 || h == _tilesHeight - 1 || w == 0 || w == _tilesWidth - 1);
            
            if (isEdge) {
                // 检查是否应该是门
                bool isDoor = false;
                int doorDir = -1;
                
                // 上方门
                if (h == _tilesHeight - 1 && _doorDirections[Direction::UP]) {
                    int doorStart = _tilesWidth / 2 - Map::HALL_WIDTH / 2;
                    int doorEnd = doorStart + Map::HALL_WIDTH - 1;
                    if (w >= doorStart && w <= doorEnd) {
                        isDoor = true;
                        doorDir = Direction::UP;
                    }
                }
                // 下方门
                if (h == 0 && _doorDirections[Direction::DOWN]) {
                    int doorStart = _tilesWidth / 2 - Map::HALL_WIDTH / 2;
                    int doorEnd = doorStart + Map::HALL_WIDTH - 1;
                    if (w >= doorStart && w <= doorEnd) {
                        isDoor = true;
                        doorDir = Direction::DOWN;
                    }
                }
                // 左方门
                if (w == 0 && _doorDirections[Direction::LEFT]) {
                    int doorStart = _tilesHeight / 2 - Map::HALL_WIDTH / 2;
                    int doorEnd = doorStart + Map::HALL_WIDTH - 1;
                    if (h >= doorStart && h <= doorEnd) {
                        isDoor = true;
                        doorDir = Direction::LEFT;
                    }
                }
                // 右方门
                if (w == _tilesWidth - 1 && _doorDirections[Direction::RIGHT]) {
                    int doorStart = _tilesHeight / 2 - Map::HALL_WIDTH / 2;
                    int doorEnd = doorStart + Map::HALL_WIDTH - 1;
                    if (h >= doorStart && h <= doorEnd) {
                        isDoor = true;
                        doorDir = Direction::RIGHT;
                    }
                }
                
                if (isDoor) {
                    generateDoor(curX, curY, doorDir);
                } else {
                    // 生成墙壁
                    bool hasShadow = (h == _tilesHeight - 1);  // 顶部墙有阴影
                    int zOrder = (h == _tilesHeight - 1) ? ZOrder::WALL_BELOW : ZOrder::WALL_ABOVE;
                    generateWall(curX, curY, zOrder, hasShadow);
                }
            } else {
                // 生成地板
                generateFloor(curX, curY);
            }
            
            curX += Map::FLOOR_WIDTH;
        }
        curX = startX;
        curY -= Map::FLOOR_HEIGHT;
    }
}

void Room::generateFloor(float x, float y) {
    // 随机选择地板贴图
    std::string floorPath = "res/floor.png";
    
    auto floor = Sprite::create(floorPath);
    if (!floor) {
        // 如果没有贴图，创建一个简单的颜色块
        floor = Sprite::create();
        floor->setTextureRect(Rect(0, 0, Map::FLOOR_WIDTH, Map::FLOOR_HEIGHT));
        floor->setColor(Color3B(60, 60, 80));
    }
    
    floor->setPosition(Vec2(x, y));
    this->addChild(floor, ZOrder::FLOOR);
    floor->setGlobalZOrder(ZOrder::FLOOR);
    _floors.pushBack(floor);
}

void Room::generateWall(float x, float y, int zOrder, bool hasShadow) {
    std::string wallPath = "res/wall.png";
    
    auto wall = Sprite::create(wallPath);
    if (!wall) {
        wall = Sprite::create();
        wall->setTextureRect(Rect(0, 0, Map::FLOOR_WIDTH, Map::WALL_HEIGHT));
        wall->setColor(Color3B(80, 80, 100));
    }
    
    wall->setPosition(Vec2(x, y));
    this->addChild(wall, zOrder);
    wall->setGlobalZOrder(zOrder);
    _walls.pushBack(wall);
    
    // 添加阴影
    if (hasShadow) {
        auto shadow = Sprite::create();
        shadow->setTextureRect(Rect(0, 0, Map::FLOOR_WIDTH, 8));
        shadow->setColor(Color3B(0, 0, 0));
        shadow->setOpacity(80);
        shadow->setPosition(Vec2(x, y - Map::FLOOR_HEIGHT / 2 - 4));
        shadow->setGlobalZOrder(ZOrder::SHADOW);
        this->addChild(shadow, ZOrder::SHADOW);
    }
}

void Room::generateDoor(float x, float y, int direction) {
    // 打开状态的门（地板样式）
    auto doorOpen = Sprite::create("res/door_open.png");
    if (!doorOpen) {
        doorOpen = Sprite::create();
        doorOpen->setTextureRect(Rect(0, 0, Map::FLOOR_WIDTH, Map::FLOOR_HEIGHT));
        doorOpen->setColor(Color3B(50, 50, 70));
    }
    doorOpen->setPosition(Vec2(x, y));
    doorOpen->setGlobalZOrder(ZOrder::DOOR);
    this->addChild(doorOpen, ZOrder::DOOR);
    _doorsOpen_sprites.pushBack(doorOpen);
    
    // 关闭状态的门（墙样式）
    auto doorClosed = Sprite::create("res/door_closed.png");
    if (!doorClosed) {
        doorClosed = Sprite::create();
        doorClosed->setTextureRect(Rect(0, 0, Map::FLOOR_WIDTH, Map::WALL_HEIGHT));
        doorClosed->setColor(Color3B(120, 60, 30));
    }
    doorClosed->setPosition(Vec2(x, y + Map::FLOOR_HEIGHT / 4));
    doorClosed->setGlobalZOrder(ZOrder::WALL_ABOVE);
    doorClosed->setVisible(false);  // 初始状态门是打开的
    this->addChild(doorClosed, ZOrder::WALL_ABOVE);
    _doorsClosed_sprites.pushBack(doorClosed);
}

void Room::setDoorOpen(int direction, bool open) {
    if (direction >= 0 && direction < Direction::COUNT) {
        _doorDirections[direction] = open;
    }
}

bool Room::connectRooms(Room* fromRoom, Room* toRoom, int direction) {
    if (!fromRoom || !toRoom) return false;
    
    // 设置双向门连接
    fromRoom->setDoorOpen(direction, true);
    int oppositeDir = (direction + 2) % Direction::COUNT;
    toRoom->setDoorOpen(oppositeDir, true);
    
    return true;
}

void Room::addEnemy(Enemy* enemy) {
    if (enemy) {
        _enemies.pushBack(enemy);
    }
}

void Room::createEnemies(int count) {
    // 随机生成敌人位置
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

void Room::removeDeadEnemies() {
    // 使用反向遍历安全删除
    for (int i = static_cast<int>(_enemies.size()) - 1; i >= 0; i--) {
        auto enemy = _enemies.at(i);
        if (enemy->getHP() <= 0 && enemy->getParent() != nullptr) {
            enemy->removeFromParent();
            _enemies.erase(i);
        }
    }
}

bool Room::allEnemiesKilled() const {
    // 起始房间没有敌人
    if (_roomType == RoomType::BEGIN) {
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
    for (auto& sprite : _doorsOpen_sprites) {
        sprite->setVisible(true);
    }
    for (auto& sprite : _doorsClosed_sprites) {
        sprite->setVisible(false);
    }
}

void Room::closeDoors() {
    _doorsOpen = false;
    for (auto& sprite : _doorsOpen_sprites) {
        sprite->setVisible(false);
    }
    for (auto& sprite : _doorsClosed_sprites) {
        sprite->setVisible(true);
    }
}

bool Room::isPlayerInRoom(Player* player) const {
    if (!player) return false;
    
    Vec2 pos = player->getPosition();
    return (pos.x >= _leftX && pos.x <= _rightX &&
            pos.y >= _bottomY && pos.y <= _topY);
}

Rect Room::getRoomBounds() const {
    float width = _rightX - _leftX;
    float height = _topY - _bottomY;
    return Rect(_leftX, _bottomY, width, height);
}

Rect Room::getWalkableArea() const {
    // 缩小一点，留出墙壁的空间
    float margin = Map::FLOOR_WIDTH;
    float width = _rightX - _leftX - margin * 2;
    float height = _topY - _bottomY - margin * 2;
    return Rect(_leftX + margin, _bottomY + margin, width, height);
}

void Room::moveBy(float dx, float dy) {
    _centerX += dx;
    _centerY += dy;
    _leftX += dx;
    _rightX += dx;
    _topY += dy;
    _bottomY += dy;
    
    // 移动所有子节点
    for (auto child : this->getChildren()) {
        Vec2 pos = child->getPosition();
        child->setPosition(pos.x + dx, pos.y + dy);
    }
}

Vec2 Room::getDoorPosition(int direction) const {
    switch (direction) {
        case Direction::UP:
            return Vec2(_centerX, _topY + Map::FLOOR_HEIGHT);
        case Direction::DOWN:
            return Vec2(_centerX, _bottomY - Map::FLOOR_HEIGHT);
        case Direction::LEFT:
            return Vec2(_leftX - Map::FLOOR_WIDTH, _centerY);
        case Direction::RIGHT:
            return Vec2(_rightX + Map::FLOOR_WIDTH, _centerY);
        default:
            return getCenter();
    }
}
