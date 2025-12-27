#include "MapGenerator.h"
#include "Entities/Player/Player.h"
#include "Hallway.h"
#include <algorithm>
#include <ctime>
#include <cstdlib>

USING_NS_CC;

// 方向偏移数组
static const int DIR_DX[] = {0, 1, 0, -1};
static const int DIR_DY[] = {1, 0, -1, 0};

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
    
    for (int y = 0; y < Constants::MAP_GRID_SIZE; y++) {
        for (int x = 0; x < Constants::MAP_GRID_SIZE; x++) {
            _roomMatrix[x][y] = nullptr;
        }
    }
    
    _roomCount = 0;
    _beginRoom = nullptr;
    _endRoom = nullptr;
    _currentRoom = nullptr;
    _levelNumber = 1;
    _isBossFloor = false;
    
    srand(static_cast<unsigned int>(time(nullptr)));
    
    return true;
}

void MapGenerator::update(float delta) {
    // 无需更新逻辑
}

void MapGenerator::generateMap() {
    clearMap();
    
    // Boss层特殊处理：只生成起始房间+Boss房间
    if (_isBossFloor) {
        generateBossFloor();
        return;
    }
    
    // 普通关卡的生成逻辑
    int startX = Constants::MAP_GRID_SIZE / 2;
    int startY = 1 + rand() % 3;
    
    randomGenerate(startX, startY);
    assignRoomTypes();
    connectAdjacentRooms();
    
    // 先创建房间地图（确保房间尺寸已根据类型调整）
    for (int y = 0; y < Constants::MAP_GRID_SIZE; y++) {
        for (int x = 0; x < Constants::MAP_GRID_SIZE; x++) {
            Room* room = _roomMatrix[x][y];
            if (room) {
                room->createMap();
                this->addChild(room);
                // 普通战斗房间应用随机地形布局
                if (room->getRoomType() == Constants::RoomType::NORMAL) {
                    TerrainLayout layout = pickRandomTerrainLayout();
                    room->applyTerrainLayout(layout);
                }
                // 奖励房间生成宝箱
                else if (room->getRoomType() == Constants::RoomType::REWARD) {
                    room->createChest();
                }
                // 终点房间生成传送门
                else if (room->getRoomType() == Constants::RoomType::END) {
                    room->createPortal();
                }
                // boss房间不生成传送门，由GameScene生成boss
            }
        }
    }
    
    // 在房间尺寸确定后再生成走廊
    generateHallways();
    
    // 添加走廊到场景
    for (auto hallway : _hallways) {
        hallway->createMap();
        this->addChild(hallway);
    }
    
    _currentRoom = _beginRoom;
    
    log("MapGenerator: Generated %d rooms and %d hallways", _roomCount, static_cast<int>(_hallways.size()));
}

// 随机选择普通战斗房间地形布局（概率：空10%，其余各9%）
TerrainLayout MapGenerator::pickRandomTerrainLayout() const {
    int r = rand() % 100; // 0..99
    if (r < 10) {
        return TerrainLayout::NONE; // 空 10%
    }
    static const TerrainLayout layouts[10] = {
        TerrainLayout::FIVE_BOXES,
        TerrainLayout::NINE_BOXES,
        TerrainLayout::UPDOWN_SPIKES,
        TerrainLayout::LEFTRIGHT_SPIKES,
        TerrainLayout::ALL_SPIKES,
        TerrainLayout::UPDOWN_BOXES,
        TerrainLayout::LEFTRIGHT_BOXES,
        TerrainLayout::CENTER_PILLAR,
        TerrainLayout::FOUR_PILLARS,
        TerrainLayout::RANDOM_MESS
    };
    int idx = (r - 10) / 9; // 将剩余90范围均分为10段，每段9%
    if (idx < 0) idx = 0; if (idx > 9) idx = 9;
    return layouts[idx];
}

void MapGenerator::randomGenerate(int startX, int startY) {
    Size visibleSize = Director::getInstance()->getVisibleSize();
    
    std::queue<Room*> q;
    
    Room* startRoom = Room::create();
    startRoom->setGridPosition(startX, startY);
    startRoom->setCenter(visibleSize.width / 2, visibleSize.height / 2);
    
    _roomMatrix[startX][startY] = startRoom;
    _beginRoom = startRoom;
    _roomCount = 1;
    
    q.push(startRoom);
    
    while (!q.empty() && _roomCount < Constants::MAP_MAX_ROOMS) {
        Room* curRoom = q.front();
        q.pop();
        expandFromRoom(curRoom->getGridX(), curRoom->getGridY(), curRoom, q);
    }
}

void MapGenerator::expandFromRoom(int x, int y, Room* curRoom, std::queue<Room*>& q) {
    if (_roomCount >= Constants::MAP_MAX_ROOMS) return;
    
    std::vector<int> availableDirections;
    
    for (int dir = 0; dir < Constants::DIR_COUNT; dir++) {
        int toX = x + DIR_DX[dir];
        int toY = y + DIR_DY[dir];
        
        if (toX < 0 || toX >= Constants::MAP_GRID_SIZE || 
            toY < 0 || toY >= Constants::MAP_GRID_SIZE) {
            continue;
        }
        
        if (_roomMatrix[toX][toY] != nullptr) {
            continue;
        }
        
        if (curRoom == _beginRoom) {
            if ((y < 2 && dir == Constants::DIR_DOWN) || 
                (y > 2 && dir == Constants::DIR_UP)) {
                continue;
            }
        }
        
        availableDirections.push_back(dir);
    }
    
    if (availableDirections.empty()) return;
    
    int expandCount = std::min(2, static_cast<int>(availableDirections.size()));
    expandCount = std::max(1, rand() % (expandCount + 1));
    
    std::random_shuffle(availableDirections.begin(), availableDirections.end());
    
    for (int i = 0; i < expandCount && _roomCount < Constants::MAP_MAX_ROOMS; i++) {
        int dir = availableDirections[i];
        int toX = x + DIR_DX[dir];
        int toY = y + DIR_DY[dir];
        
        if (_roomMatrix[toX][toY] != nullptr) continue;
        
        Vec2 curCenter = curRoom->getCenter();
        float newCenterX = curCenter.x + DIR_DX[dir] * Constants::ROOM_CENTER_DIST;
        float newCenterY = curCenter.y + DIR_DY[dir] * Constants::ROOM_CENTER_DIST;
        
        Room* newRoom = Room::create();
        newRoom->setGridPosition(toX, toY);
        newRoom->setCenter(newCenterX, newCenterY);
        
        _roomMatrix[toX][toY] = newRoom;
        _endRoom = newRoom;
        _roomCount++;
        
        q.push(newRoom);
        
        log("Created room at (%d, %d), total: %d", toX, toY, _roomCount);
    }
}

void MapGenerator::assignRoomTypes() {
    std::vector<Room*> normalRooms;
    
    for (int y = 0; y < Constants::MAP_GRID_SIZE; y++) {
        for (int x = 0; x < Constants::MAP_GRID_SIZE; x++) {
            Room* room = _roomMatrix[x][y];
            if (room == nullptr) continue;
            
            if (room == _beginRoom) {
                room->setRoomType(Constants::RoomType::BEGIN);
                room->setVisited(true);
            } else if (room == _endRoom) {
                // 每5关一个boss（如果需要），否则是终点房间
                if (_levelNumber % 5 == 0) {
                    room->setRoomType(Constants::RoomType::BOSS);
                } else {
                    room->setRoomType(Constants::RoomType::END);
                }
            } else {
                room->setRoomType(Constants::RoomType::NORMAL);
                
                bool connectedToBegin = false;
                for (int dir = 0; dir < Constants::DIR_COUNT; dir++) {
                    int checkX = x + DIR_DX[dir];
                    int checkY = y + DIR_DY[dir];
                    if (checkX >= 0 && checkX < Constants::MAP_GRID_SIZE &&
                        checkY >= 0 && checkY < Constants::MAP_GRID_SIZE) {
                        if (_roomMatrix[checkX][checkY] == _beginRoom) {
                            connectedToBegin = true;
                            break;
                        }
                    }
                }
                
                if (!connectedToBegin) {
                    normalRooms.push_back(room);
                }
            }
        }
    }
    
    std::random_shuffle(normalRooms.begin(), normalRooms.end());
    
    if (normalRooms.size() >= 1) {
        normalRooms[0]->setRoomType(Constants::RoomType::REWARD);
    }
    if (normalRooms.size() >= 2) {
        normalRooms[1]->setRoomType(Constants::RoomType::REWARD);
    }
}

void MapGenerator::connectAdjacentRooms() {
    for (int y = 0; y < Constants::MAP_GRID_SIZE; y++) {
        for (int x = 0; x < Constants::MAP_GRID_SIZE; x++) {
            Room* room = _roomMatrix[x][y];
            if (room == nullptr) continue;
            
            for (int dir = 0; dir < Constants::DIR_COUNT; dir++) {
                int toX = x + DIR_DX[dir];
                int toY = y + DIR_DY[dir];
                
                if (toX < 0 || toX >= Constants::MAP_GRID_SIZE || 
                    toY < 0 || toY >= Constants::MAP_GRID_SIZE) {
                    continue;
                }
                
                Room* adjacentRoom = _roomMatrix[toX][toY];
                if (adjacentRoom != nullptr) {
                    // 如果是BEGIN和END房间相邻，不设置门为开启
                    bool isBeginEndConnection = 
                        (room == _beginRoom && adjacentRoom == _endRoom) ||
                        (room == _endRoom && adjacentRoom == _beginRoom);
                    
                    if (!isBeginEndConnection) {
                        room->setDoorOpen(dir, true);
                    }
                }
            }
        }
    }
}

void MapGenerator::generateHallways() {
    _hallways.clear();
    
    float tileSize = Constants::FLOOR_TILE_SIZE;
    
    for (int y = 0; y < Constants::MAP_GRID_SIZE; y++) {
        for (int x = 0; x < Constants::MAP_GRID_SIZE; x++) {
            Room* room = _roomMatrix[x][y];
            if (room == nullptr) continue;
            
            Vec2 roomCenter = room->getCenter();
            float roomWidth = room->getTilesWidth() * tileSize;
            float roomHeight = room->getTilesHeight() * tileSize;
            
            // 只检查右边和下边，避免重复生成
            // 检查右边（DIR_RIGHT）
            if (room->hasDoor(Constants::DIR_RIGHT)) {
                int toX = x + DIR_DX[Constants::DIR_RIGHT];
                int toY = y + DIR_DY[Constants::DIR_RIGHT];
                
                if (toX < Constants::MAP_GRID_SIZE && _roomMatrix[toX][toY] != nullptr) {
                    Room* rightRoom = _roomMatrix[toX][toY];
                    
                    // 如果是BEGIN和END房间相邻，不生成走廊
                    bool isBeginEndConnection = 
                        (room == _beginRoom && rightRoom == _endRoom) ||
                        (room == _endRoom && rightRoom == _beginRoom);
                    
                    if (!isBeginEndConnection) {
                        Vec2 rightCenter = rightRoom->getCenter();
                        float rightRoomWidth = rightRoom->getTilesWidth() * tileSize;
                        
                        // 水平走廊：在两个房间的右/左边缘之间
                        float leftRoomRightEdge = roomCenter.x + roomWidth / 2.0f;
                        float rightRoomLeftEdge = rightCenter.x - rightRoomWidth / 2.0f;
                        float hallwayCenterX = (leftRoomRightEdge + rightRoomLeftEdge) / 2.0f;
                        float hallwayCenterY = roomCenter.y;
                        float gapSize = rightRoomLeftEdge - leftRoomRightEdge;  // 实际空隙
                        
                        Hallway* hallway = Hallway::create(Constants::DIR_RIGHT);
                        hallway->setGapSize(gapSize);
                        hallway->setCenter(hallwayCenterX, hallwayCenterY);
                        _hallways.push_back(hallway);
                        log("Generated RIGHT hallway at (%.1f, %.1f) gap=%.1f connecting (%d,%d) -> (%d,%d)",
                            hallwayCenterX, hallwayCenterY, gapSize, x, y, toX, toY);
                    }
                }
            }
            
            // 检查下边（DIR_DOWN）
            if (room->hasDoor(Constants::DIR_DOWN)) {
                int toX = x + DIR_DX[Constants::DIR_DOWN];
                int toY = y + DIR_DY[Constants::DIR_DOWN];
                
                if (toY >= 0 && toY < Constants::MAP_GRID_SIZE && _roomMatrix[toX][toY] != nullptr) {
                    Room* downRoom = _roomMatrix[toX][toY];
                    
                    // 如果是BEGIN和END房间相邻，不生成走廊
                    bool isBeginEndConnection = 
                        (room == _beginRoom && downRoom == _endRoom) ||
                        (room == _endRoom && downRoom == _beginRoom);
                    
                    if (!isBeginEndConnection) {
                        Vec2 downCenter = downRoom->getCenter();
                        float downRoomHeight = downRoom->getTilesHeight() * tileSize;
                        
                        // 垂直走廊：在两个房间的上/下边缘之间
                        float topRoomBottomEdge = roomCenter.y - roomHeight / 2.0f;
                        float bottomRoomTopEdge = downCenter.y + downRoomHeight / 2.0f;
                        float hallwayCenterY = (topRoomBottomEdge + bottomRoomTopEdge) / 2.0f;
                        float hallwayCenterX = roomCenter.x;
                        float gapSize = topRoomBottomEdge - bottomRoomTopEdge;  // 实际空隙
                        
                        Hallway* hallway = Hallway::create(Constants::DIR_DOWN);
                        hallway->setGapSize(gapSize);
                        hallway->setCenter(hallwayCenterX, hallwayCenterY);
                        _hallways.push_back(hallway);
                        log("Generated DOWN hallway at (%.1f, %.1f) gap=%.1f connecting (%d,%d) -> (%d,%d)",
                            hallwayCenterX, hallwayCenterY, gapSize, x, y, toX, toY);
                    }
                }
            }
        }
    }
}

Room* MapGenerator::getRoom(int x, int y) {
    if (x < 0 || x >= Constants::MAP_GRID_SIZE || 
        y < 0 || y >= Constants::MAP_GRID_SIZE) {
        return nullptr;
    }
    return _roomMatrix[x][y];
}

void MapGenerator::setCurrentRoom(Room* room) {
    _currentRoom = room;
}

Room* MapGenerator::updatePlayerRoom(Player* player) {
    if (!player) return _currentRoom;
    
    for (int y = 0; y < Constants::MAP_GRID_SIZE; y++) {
        for (int x = 0; x < Constants::MAP_GRID_SIZE; x++) {
            Room* room = _roomMatrix[x][y];
            if (room && room->isPlayerInRoom(player)) {
                if (room != _currentRoom) {
                    _currentRoom = room;
                    room->setVisited(true);
                    
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
    for (int y = 0; y < Constants::MAP_GRID_SIZE; y++) {
        for (int x = 0; x < Constants::MAP_GRID_SIZE; x++) {
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

Hallway* MapGenerator::getPlayerHallway(Player* player) {
    if (!player) return nullptr;
    
    for (auto hallway : _hallways) {
        if (hallway && hallway->isPlayerInHallway(player)) {
            return hallway;
        }
    }
    return nullptr;
}

std::vector<Room*> MapGenerator::getAllRooms() const {
    std::vector<Room*> rooms;
    for (int y = 0; y < Constants::MAP_GRID_SIZE; y++) {
        for (int x = 0; x < Constants::MAP_GRID_SIZE; x++) {
            if (_roomMatrix[x][y] != nullptr) {
                rooms.push_back(_roomMatrix[x][y]);
            }
        }
    }
    return rooms;
}

void MapGenerator::clearMap() {
    for (int y = 0; y < Constants::MAP_GRID_SIZE; y++) {
        for (int x = 0; x < Constants::MAP_GRID_SIZE; x++) {
            if (_roomMatrix[x][y]) {
                _roomMatrix[x][y]->removeFromParent();
                _roomMatrix[x][y] = nullptr;
            }
        }
    }
    
    // 清理走廊
    for (auto hallway : _hallways) {
        hallway->removeFromParent();
    }
    _hallways.clear();
    
    _roomCount = 0;
    _beginRoom = nullptr;
    _endRoom = nullptr;
    _currentRoom = nullptr;
}

/**
 * 生成Boss层地图
 * 
 * Boss层结构说明：
 * - 只有2个房间：起始房间(左) + Boss房间(右)
 * - Boss房间大小为普通房间的2倍 (56x40 瓦片)
 * - Boss房间会随机生成30个火焰地板
 * - Boss房间中心会生成Boss敌人 (KuiLongBoss)
 * 
 * 修改Boss房间布局请编辑此方法
 * 修改Boss敌人生成请编辑 GameScene::spawnEnemiesInRoom()
 * 
 * @see Room::generateBossFloorTiles() - Boss房间地板生成
 * @see GameScene::spawnEnemiesInRoom() - Boss敌人生成
 */
void MapGenerator::generateBossFloor() {
    Size visibleSize = Director::getInstance()->getVisibleSize();
    int startX = Constants::MAP_GRID_SIZE / 2;
    int startY = 2; // 固定在中间位置
    
    // ========== 创建起始房间 ==========
    Room* startRoom = Room::create();
    startRoom->setGridPosition(startX, startY);
    startRoom->setCenter(visibleSize.width / 2, visibleSize.height / 2);
    _roomMatrix[startX][startY] = startRoom;
    _beginRoom = startRoom;
    _roomCount = 1;
    
    // ========== 创建Boss房间 ==========
    // Boss房间位于起始房间右侧
    int bossX = startX + 1;
    int bossY = startY;
    Room* bossRoom = Room::create();
    bossRoom->setGridPosition(bossX, bossY);
    
    // Boss房间中心位置计算
    // 注意：Boss房间是2倍大小，需要考虑房间边缘对齐
    float startRoomRightEdge = visibleSize.width / 2 + Constants::ROOM_TILES_W * Constants::FLOOR_TILE_SIZE / 2;
    float bossRoomHalfWidth = Constants::ROOM_TILES_W * 2 * Constants::FLOOR_TILE_SIZE / 2;
    float bossRoomCenterX = startRoomRightEdge + bossRoomHalfWidth;
    bossRoom->setCenter(bossRoomCenterX, visibleSize.height / 2);
    
    _roomMatrix[bossX][bossY] = bossRoom;
    _endRoom = bossRoom;
    _roomCount = 2;
    
    // ========== 设置房间类型 ==========
    startRoom->setRoomType(Constants::RoomType::BEGIN);
    startRoom->setVisited(true);
    bossRoom->setRoomType(Constants::RoomType::BOSS);
    
    // ========== 连接两个房间的门 ==========
    startRoom->setDoorOpen(Constants::DIR_RIGHT, true);
    bossRoom->setDoorOpen(Constants::DIR_LEFT, true);
    
    // ========== 创建房间地图 ==========
    startRoom->createMap();
    bossRoom->createMap();
    
    // Boss房间生成火焰地板装饰 (50个火焰地板)
    bossRoom->generateBossFloorTiles(50);
    log("Boss room fire tiles generated, center=(%.1f, %.1f)", bossRoom->getCenter().x, bossRoom->getCenter().y);
    
    this->addChild(startRoom);
    this->addChild(bossRoom);
    
    // ========== 生成连接走廊 ==========
    // Boss层使用水平走廊连接两个房间
    float tileSize = Constants::FLOOR_TILE_SIZE;
    float startRoomWidth = Constants::ROOM_TILES_W * tileSize;
    float bossRoomWidth = Constants::ROOM_TILES_W * 2 * tileSize;
    
    Vec2 startCenter = startRoom->getCenter();
    Vec2 bossCenter = bossRoom->getCenter();
    
    // 走廊起点：起始房间右边缘
    float hallwayStartX = startCenter.x + startRoomWidth / 2;
    // 走廊终点：Boss房间左边缘
    float hallwayEndX = bossCenter.x - bossRoomWidth / 2;
    float gapSize = hallwayEndX - hallwayStartX;
    
    // 只有当两个房间之间有间隙时才生成走廊
    if (gapSize > 0) {
        float hallwayCenterX = (hallwayStartX + hallwayEndX) / 2.0f;
        float hallwayCenterY = startCenter.y;
        
        Hallway* hallway = Hallway::create(Constants::DIR_RIGHT);
        hallway->setGapSize(gapSize);
        hallway->setCenter(hallwayCenterX, hallwayCenterY);
        hallway->createMap();
        _hallways.push_back(hallway);
        this->addChild(hallway, Constants::ZOrder::FLOOR);
    }
    
    log("Boss floor generated: Start room at (%d,%d), Boss room at (%d,%d)", 
        startX, startY, bossX, bossY);
}
