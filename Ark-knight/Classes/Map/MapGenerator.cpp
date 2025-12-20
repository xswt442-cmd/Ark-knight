#include "MapGenerator.h"
#include "Entities/Player/Player.h"
#include <algorithm>
#include <ctime>
#include <cstdlib>

USING_NS_CC;

using namespace Constants;

MapGenerator* MapGenerator::create() {
    MapGenerator* generator = new (std::nothrow) MapGenerator();
    if (generator && generator->init()) {
        generator->autorelease();
        return generator;
    }
    CC_SAFE_DELETE(generator);
    return nullptr;
}

bool MapGenerator::init() {
    if (!Node::init()) {
        return false;
    }
    
    // 初始化房间矩阵为nullptr
    for (int y = 0; y < Map::MAP_SIZE; y++) {
        for (int x = 0; x < Map::MAP_SIZE; x++) {
            _roomMatrix[x][y] = nullptr;
        }
    }
    
    _roomCount = 0;
    _beginRoom = nullptr;
    _endRoom = nullptr;
    _currentRoom = nullptr;
    _levelNumber = 1;
    
    // 初始化随机种子
    srand(static_cast<unsigned int>(time(nullptr)));
    
    return true;
}

void MapGenerator::update(float delta) {
    // 地图生成器本身不需要更新逻辑
}

void MapGenerator::generateMap() {
    // 清理旧地图
    clearMap();
    
    // 随机选择起始位置（中间偏上的位置）
    int startX = Map::MAP_SIZE / 2;
    int startY = 1 + rand() % 3;  // y在1-3之间
    
    // BFS生成房间
    randomGenerate(startX, startY);
    
    // 分配房间类型
    assignRoomTypes();
    
    // 连接相邻房间的门
    connectAdjacentRooms();
    
    // 为每个房间创建地图
    for (int y = 0; y < Map::MAP_SIZE; y++) {
        for (int x = 0; x < Map::MAP_SIZE; x++) {
            Room* room = _roomMatrix[x][y];
            if (room) {
                room->createMap();
                this->addChild(room);
            }
        }
    }
    
    // 设置当前房间为起始房间
    _currentRoom = _beginRoom;
    
    log("MapGenerator: Generated %d rooms", _roomCount);
}

void MapGenerator::randomGenerate(int startX, int startY) {
    Size visibleSize = Director::getInstance()->getVisibleSize();
    
    std::queue<Room*> q;
    
    // 创建起始房间
    Room* startRoom = Room::create();
    startRoom->setGridPosition(startX, startY);
    startRoom->setCenter(visibleSize.width / 2, visibleSize.height / 2);
    
    _roomMatrix[startX][startY] = startRoom;
    _beginRoom = startRoom;
    _roomCount = 1;
    
    q.push(startRoom);
    
    // BFS扩展房间
    while (!q.empty() && _roomCount < Map::MAX_ROOMS) {
        Room* curRoom = q.front();
        q.pop();
        
        expandFromRoom(curRoom->getGridX(), curRoom->getGridY(), curRoom, q);
    }
}

void MapGenerator::expandFromRoom(int x, int y, Room* curRoom, std::queue<Room*>& q) {
    if (_roomCount >= Map::MAX_ROOMS) return;
    
    // 收集可以扩展的方向
    std::vector<int> availableDirections;
    
    for (int dir = 0; dir < Direction::COUNT; dir++) {
        int toX = x + Direction::DX[dir];
        int toY = y + Direction::DY[dir];
        
        // 检查边界
        if (toX < 0 || toX >= Map::MAP_SIZE || toY < 0 || toY >= Map::MAP_SIZE) {
            continue;
        }
        
        // 检查是否已有房间
        if (_roomMatrix[toX][toY] != nullptr) {
            continue;
        }
        
        // 起始房间附近不往某些方向扩展（保证布局合理）
        if (curRoom == _beginRoom) {
            if ((y < 2 && dir == Direction::DOWN) || (y > 2 && dir == Direction::UP)) {
                continue;
            }
        }
        
        availableDirections.push_back(dir);
    }
    
    if (availableDirections.empty()) return;
    
    // 随机选择1-2个方向扩展
    int expandCount = std::min(2, static_cast<int>(availableDirections.size()));
    expandCount = std::max(1, rand() % (expandCount + 1));
    
    // 随机打乱方向
    std::random_shuffle(availableDirections.begin(), availableDirections.end());
    
    for (int i = 0; i < expandCount && _roomCount < Map::MAX_ROOMS; i++) {
        int dir = availableDirections[i];
        int toX = x + Direction::DX[dir];
        int toY = y + Direction::DY[dir];
        
        // 再次检查（可能在循环中被其他分支创建）
        if (_roomMatrix[toX][toY] != nullptr) continue;
        
        // 计算新房间的中心位置
        Vec2 curCenter = curRoom->getCenter();
        float newCenterX = curCenter.x + Direction::DX[dir] * Map::CENTER_DISTANCE;
        float newCenterY = curCenter.y + Direction::DY[dir] * Map::CENTER_DISTANCE;
        
        // 创建新房间
        Room* newRoom = Room::create();
        newRoom->setGridPosition(toX, toY);
        newRoom->setCenter(newCenterX, newCenterY);
        
        _roomMatrix[toX][toY] = newRoom;
        _endRoom = newRoom;  // 最后创建的房间作为终点
        _roomCount++;
        
        q.push(newRoom);
        
        log("Created room at (%d, %d), total: %d", toX, toY, _roomCount);
    }
}

void MapGenerator::assignRoomTypes() {
    // 收集所有非起点非终点的房间
    std::vector<Room*> normalRooms;
    
    for (int y = 0; y < Map::MAP_SIZE; y++) {
        for (int x = 0; x < Map::MAP_SIZE; x++) {
            Room* room = _roomMatrix[x][y];
            if (room == nullptr) continue;
            
            if (room == _beginRoom) {
                room->setRoomType(RoomType::BEGIN);
                room->setVisited(true);  // 起始房间默认已访问
            } else if (room == _endRoom) {
                // 每5关一个Boss
                if (_levelNumber % 5 == 0) {
                    room->setRoomType(RoomType::BOSS);
                } else {
                    room->setRoomType(RoomType::END);
                }
            } else {
                room->setRoomType(RoomType::NORMAL);
                
                // 检查是否直接连接起始房间
                bool connectedToBegin = false;
                for (int dir = 0; dir < Direction::COUNT; dir++) {
                    int checkX = x + Direction::DX[dir];
                    int checkY = y + Direction::DY[dir];
                    if (checkX >= 0 && checkX < Map::MAP_SIZE &&
                        checkY >= 0 && checkY < Map::MAP_SIZE) {
                        if (_roomMatrix[checkX][checkY] == _beginRoom) {
                            connectedToBegin = true;
                            break;
                        }
                    }
                }
                
                // 不直接连接起点的房间可以作为特殊房间
                if (!connectedToBegin) {
                    normalRooms.push_back(room);
                }
            }
        }
    }
    
    // 随机设置武器和道具房间
    std::random_shuffle(normalRooms.begin(), normalRooms.end());
    
    if (normalRooms.size() >= 1) {
        normalRooms[0]->setRoomType(RoomType::WEAPON);
    }
    if (normalRooms.size() >= 2) {
        normalRooms[1]->setRoomType(RoomType::PROP);
    }
}

void MapGenerator::connectAdjacentRooms() {
    for (int y = 0; y < Map::MAP_SIZE; y++) {
        for (int x = 0; x < Map::MAP_SIZE; x++) {
            Room* room = _roomMatrix[x][y];
            if (room == nullptr) continue;
            
            // 检查四个方向的相邻房间
            for (int dir = 0; dir < Direction::COUNT; dir++) {
                int toX = x + Direction::DX[dir];
                int toY = y + Direction::DY[dir];
                
                if (toX < 0 || toX >= Map::MAP_SIZE || 
                    toY < 0 || toY >= Map::MAP_SIZE) {
                    continue;
                }
                
                Room* adjacentRoom = _roomMatrix[toX][toY];
                if (adjacentRoom != nullptr) {
                    // 设置门连接（只设置一个方向，避免重复）
                    room->setDoorOpen(dir, true);
                }
            }
        }
    }
}

Room* MapGenerator::getRoom(int x, int y) {
    if (x < 0 || x >= Map::MAP_SIZE || y < 0 || y >= Map::MAP_SIZE) {
        return nullptr;
    }
    return _roomMatrix[x][y];
}

void MapGenerator::setCurrentRoom(Room* room) {
    _currentRoom = room;
}

Room* MapGenerator::updatePlayerRoom(Player* player) {
    if (!player) return _currentRoom;
    
    // 检查所有房间，找到玩家所在的房间
    for (int y = 0; y < Map::MAP_SIZE; y++) {
        for (int x = 0; x < Map::MAP_SIZE; x++) {
            Room* room = _roomMatrix[x][y];
            if (room && room->isPlayerInRoom(player)) {
                if (room != _currentRoom) {
                    // 玩家进入新房间
                    _currentRoom = room;
                    room->setVisited(true);
                    
                    // 如果房间有敌人且未被清理，关闭门
                    if (!room->allEnemiesKilled()) {
                        room->closeDoors();
                    }
                    
                    log("Player entered room (%d, %d)", x, y);
                }
                return room;
            }
        }
    }
    
    return _currentRoom;
}

void MapGenerator::moveAllRoomsBy(float dx, float dy) {
    for (int y = 0; y < Map::MAP_SIZE; y++) {
        for (int x = 0; x < Map::MAP_SIZE; x++) {
            Room* room = _roomMatrix[x][y];
            if (room) {
                room->moveBy(dx, dy);
            }
        }
    }
}

Vec2 MapGenerator::getRoomWorldPosition(int gridX, int gridY) {
    Room* room = getRoom(gridX, gridY);
    if (room) {
        return room->getCenter();
    }
    return Vec2::ZERO;
}

void MapGenerator::clearMap() {
    for (int y = 0; y < Map::MAP_SIZE; y++) {
        for (int x = 0; x < Map::MAP_SIZE; x++) {
            if (_roomMatrix[x][y]) {
                _roomMatrix[x][y]->removeFromParent();
                _roomMatrix[x][y] = nullptr;
            }
        }
    }
    
    _roomCount = 0;
    _beginRoom = nullptr;
    _endRoom = nullptr;
    _currentRoom = nullptr;
}
