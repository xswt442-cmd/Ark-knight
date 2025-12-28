#include "BossFloor.h"

USING_NS_CC;

BossFloor* BossFloor::create() {
    BossFloor* ret = new (std::nothrow) BossFloor();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool BossFloor::init() {
    if (!Node::init()) {
        return false;
    }
    _startRoom = nullptr;
    _bossRoom = nullptr;
    _phase3Room = nullptr;
    return true;
}

// 生成Boss层地图
void BossFloor::generate(
    Room* (&roomMatrix)[Constants::MAP_GRID_SIZE][Constants::MAP_GRID_SIZE],
    std::vector<Hallway*>& hallways,
    Room*& beginRoom,
    Room*& endRoom,
    int& roomCount
) {
    Size visibleSize = Director::getInstance()->getVisibleSize();
    
    // 房间在网格中的位置
    int startGridX = Constants::MAP_GRID_SIZE / 2;
    int startGridY = 2;
    int bossGridX = startGridX + 1;
    int bossGridY = startGridY;
    int phase3GridX = bossGridX + 1; // 三阶段房间在Boss房间右侧
    int phase3GridY = startGridY;
    
    // 创建起始房间
    _startRoom = createStartRoom(startGridX, startGridY);
    roomMatrix[startGridX][startGridY] = _startRoom;
    beginRoom = _startRoom;
    
    // 创建Boss房间
    float startRoomRightEdge = visibleSize.width / 2 + 
        Constants::ROOM_TILES_W * Constants::FLOOR_TILE_SIZE / 2;
    _bossRoom = createBossRoom(bossGridX, bossGridY, startRoomRightEdge);
    roomMatrix[bossGridX][bossGridY] = _bossRoom;
    endRoom = _bossRoom; // PS: 逻辑上的终点仍是Boss房间

    // 创建三阶段房间
    // 计算Boss房间右边缘
    float bossRoomWidth = Constants::ROOM_TILES_W * 2 * Constants::FLOOR_TILE_SIZE;
    float bossRoomCenterX = _bossRoom->getCenter().x;
    float bossRoomRightEdge = bossRoomCenterX + bossRoomWidth / 2;

    _phase3Room = createPhase3Room(phase3GridX, phase3GridY, bossRoomRightEdge);
    // PS: 这里不一定要放入 roomMatrix，或者放入但 MapGenerator 不会为其生成常规走廊
    // 为了方便管理，我们放入矩阵，但因为 generateHallway 是手动调用的，所以不会自动生成走廊
    if (phase3GridX < Constants::MAP_GRID_SIZE) {
        roomMatrix[phase3GridX][phase3GridY] = _phase3Room;
    }
    
    roomCount = 3; // 增加房间计数
    
    // 连接房间门 (起始 <-> Boss)
    _startRoom->setDoorOpen(Constants::DIR_RIGHT, true);
    _bossRoom->setDoorOpen(Constants::DIR_LEFT, true);
    // PS: Boss房间与三阶段房间之间不设门，也不设走廊
    
    // 创建房间地图
    _startRoom->createMap();
    _bossRoom->createMap();
    _phase3Room->createMap();
    
    this->addChild(_startRoom);
    this->addChild(_bossRoom);
    this->addChild(_phase3Room);
    
    // 生成火焰地板装饰
    generateFireTiles();
    
    // 生成连接走廊 (仅起始 <-> Boss)
    generateHallway(hallways);
    
    log("Boss floor generated: Start(%d,%d), Boss(%d,%d), Phase3(%d,%d)", 
        startGridX, startGridY, bossGridX, bossGridY, phase3GridX, phase3GridY);
}

Room* BossFloor::createStartRoom(int gridX, int gridY) {
    Size visibleSize = Director::getInstance()->getVisibleSize();
    
    Room* room = Room::create();
    room->setGridPosition(gridX, gridY);
    room->setCenter(visibleSize.width / 2, visibleSize.height / 2);
    room->setRoomType(Constants::RoomType::BEGIN);
    room->setVisited(true);
    
    return room;
}

Room* BossFloor::createBossRoom(int gridX, int gridY, float startRoomRightEdge) {
    Size visibleSize = Director::getInstance()->getVisibleSize();
    
    // Boss房间是2倍大小
    float bossRoomHalfWidth = Constants::ROOM_TILES_W * 2 * Constants::FLOOR_TILE_SIZE / 2;
    float bossRoomCenterX = startRoomRightEdge + bossRoomHalfWidth;
    
    Room* room = Room::create();
    room->setGridPosition(gridX, gridY);
    room->setCenter(bossRoomCenterX, visibleSize.height / 2);
    room->setRoomType(Constants::RoomType::BOSS);
    
    return room;
}

// 创建三阶段房间，大小为普通房间大小
Room* BossFloor::createPhase3Room(int gridX, int gridY, float bossRoomRightEdge) {
    Size visibleSize = Director::getInstance()->getVisibleSize();
    
    // 三阶段房间大小为普通房间大小 (Boss房间的一半)
    // 为了视觉上隔开，增加一点间距
    float gap = Constants::FLOOR_TILE_SIZE * 5.0f; 
    float roomHalfWidth = Constants::ROOM_TILES_W * Constants::FLOOR_TILE_SIZE / 2;
    float roomCenterX = bossRoomRightEdge + gap + roomHalfWidth;
    
    Room* room = Room::create();
    room->setGridPosition(gridX, gridY);
    room->setCenter(roomCenterX, visibleSize.height / 2);
    // 设置为普通类型或特殊类型，这里用 NORMAL 即可，因为不需要特殊生成逻辑，只是作为场地
    // 或者可以定义一个新的 RoomType::PHASE3 如果需要特殊处理
    room->setRoomType(Constants::RoomType::NORMAL); 
    
    return room;
}

void BossFloor::generateFireTiles() {
    if (!_bossRoom) return;
    
    Vec2 bossCenter = _bossRoom->getCenter();
    int roomTilesW = _bossRoom->getTilesWidth();
    int roomTilesH = _bossRoom->getTilesHeight();
    float tileSize = Constants::FLOOR_TILE_SIZE;
    
    float baseX = bossCenter.x - tileSize * (roomTilesW / 2.0f - 0.5f);
    float baseY = bossCenter.y + tileSize * (roomTilesH / 2.0f - 0.5f);
    
    std::vector<std::pair<int, int>> availablePositions;
    int centerTileX = roomTilesW / 2;
    int centerTileY = roomTilesH / 2;
    
    for (int y = FIRE_EDGE_MARGIN; y < roomTilesH - FIRE_EDGE_MARGIN; ++y) {
        for (int x = FIRE_EDGE_MARGIN; x < roomTilesW - FIRE_EDGE_MARGIN; ++x) {
            int dx = x - centerTileX;
            int dy = y - centerTileY;
            if (dx * dx + dy * dy <= FIRE_EXCLUSION_RADIUS * FIRE_EXCLUSION_RADIUS) {
                continue;
            }
            availablePositions.push_back({x, y});
        }
    }
    
    for (size_t i = availablePositions.size() - 1; i > 0; --i) {
        size_t j = rand() % (i + 1);
        std::swap(availablePositions[i], availablePositions[j]);
    }
    
    int fireCount = std::min(FIRE_TILE_COUNT, (int)availablePositions.size());
    for (int i = 0; i < fireCount; ++i) {
        int tileX = availablePositions[i].first;
        int tileY = availablePositions[i].second;
        
        float posX = baseX + tileX * tileSize;
        float posY = baseY - tileY * tileSize;
        
        auto fireFloor = Sprite::create("Map/Floor/Floor_fire.png");
        if (fireFloor) {
            fireFloor->setPosition(posX, posY);
            fireFloor->setGlobalZOrder(Constants::ZOrder::FLOOR + 2);
            this->addChild(fireFloor, Constants::ZOrder::FLOOR + 2);
            _fireFloors.pushBack(fireFloor);
        }
    }
}

void BossFloor::generateHallway(std::vector<Hallway*>& hallways) {
    if (!_startRoom || !_bossRoom) return;
    
    float tileSize = Constants::FLOOR_TILE_SIZE;
    float startRoomWidth = Constants::ROOM_TILES_W * tileSize;
    float bossRoomWidth = Constants::ROOM_TILES_W * 2 * tileSize;
    
    Vec2 startCenter = _startRoom->getCenter();
    Vec2 bossCenter = _bossRoom->getCenter();
    
    float hallwayStartX = startCenter.x + startRoomWidth / 2;
    float hallwayEndX = bossCenter.x - bossRoomWidth / 2;
    float gapSize = hallwayEndX - hallwayStartX;
    
    if (gapSize > 0) {
        float hallwayCenterX = (hallwayStartX + hallwayEndX) / 2.0f;
        float hallwayCenterY = startCenter.y;
        
        Hallway* hallway = Hallway::create(Constants::DIR_RIGHT);
        hallway->setGapSize(gapSize);
        hallway->setCenter(hallwayCenterX, hallwayCenterY);
        hallway->createMap();
        hallways.push_back(hallway);
        this->addChild(hallway, Constants::ZOrder::FLOOR);
        
        log("Generated hallway between start room and boss room, gap=%.1f", gapSize);
    }
    
    // 注意：这里不生成 BossRoom 和 Phase3Room 之间的走廊
}
