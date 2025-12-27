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
    return true;
}

/**
 * 生成Boss层地图
 * 
 * Boss层结构：
 * ┌─────────┐    ┌───────────────────┐
 * │  起始   │════│      Boss房间      │
 * │  房间   │    │    (2倍大小)       │
 * └─────────┘    └───────────────────┘
 */
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
    
    // ========== 创建起始房间 ==========
    _startRoom = createStartRoom(startGridX, startGridY);
    roomMatrix[startGridX][startGridY] = _startRoom;
    beginRoom = _startRoom;
    
    // ========== 创建Boss房间 ==========
    float startRoomRightEdge = visibleSize.width / 2 + 
        Constants::ROOM_TILES_W * Constants::FLOOR_TILE_SIZE / 2;
    _bossRoom = createBossRoom(bossGridX, bossGridY, startRoomRightEdge);
    roomMatrix[bossGridX][bossGridY] = _bossRoom;
    endRoom = _bossRoom;
    
    roomCount = 2;
    
    // ========== 连接两个房间的门 ==========
    _startRoom->setDoorOpen(Constants::DIR_RIGHT, true);
    _bossRoom->setDoorOpen(Constants::DIR_LEFT, true);
    
    // ========== 创建房间地图 ==========
    _startRoom->createMap();
    _bossRoom->createMap();
    
    this->addChild(_startRoom);
    this->addChild(_bossRoom);
    
    // ========== 生成火焰地板装饰 ==========
    generateFireTiles();
    
    // ========== 生成连接走廊 ==========
    generateHallway(hallways);
    
    log("Boss floor generated: Start room at (%d,%d), Boss room at (%d,%d)", 
        startGridX, startGridY, bossGridX, bossGridY);
}

/**
 * 创建起始房间
 * 
 * @param gridX 网格X坐标
 * @param gridY 网格Y坐标
 * @return 创建的房间指针
 */
Room* BossFloor::createStartRoom(int gridX, int gridY) {
    Size visibleSize = Director::getInstance()->getVisibleSize();
    
    Room* room = Room::create();
    room->setGridPosition(gridX, gridY);
    room->setCenter(visibleSize.width / 2, visibleSize.height / 2);
    room->setRoomType(Constants::RoomType::BEGIN);
    room->setVisited(true);
    
    return room;
}

/**
 * 创建Boss房间
 * 
 * Boss房间特点：
 * - 大小为普通房间的2倍 (在Room::createMap中根据BOSS类型自动设置)
 * - 位于起始房间右侧
 * 
 * @param gridX 网格X坐标
 * @param gridY 网格Y坐标
 * @param startRoomRightEdge 起始房间右边缘的X坐标
 * @return 创建的房间指针
 */
Room* BossFloor::createBossRoom(int gridX, int gridY, float startRoomRightEdge) {
    Size visibleSize = Director::getInstance()->getVisibleSize();
    
    // Boss房间是2倍大小，计算中心位置
    float bossRoomHalfWidth = Constants::ROOM_TILES_W * 2 * Constants::FLOOR_TILE_SIZE / 2;
    float bossRoomCenterX = startRoomRightEdge + bossRoomHalfWidth;
    
    Room* room = Room::create();
    room->setGridPosition(gridX, gridY);
    room->setCenter(bossRoomCenterX, visibleSize.height / 2);
    room->setRoomType(Constants::RoomType::BOSS);
    
    return room;
}

/**
 * 生成火焰地板装饰
 * 
 * 在Boss房间内随机生成火焰地板，用于增加视觉效果
 * 火焰地板不会生成在：
 * - 房间边缘（FIRE_EDGE_MARGIN 瓦片范围内）
 * - Boss中心区域（FIRE_EXCLUSION_RADIUS 瓦片范围内）
 * 
 * 调整参数请修改头文件中的常量
 */
void BossFloor::generateFireTiles() {
    if (!_bossRoom) return;
    
    Vec2 bossCenter = _bossRoom->getCenter();
    int roomTilesW = _bossRoom->getTilesWidth();
    int roomTilesH = _bossRoom->getTilesHeight();
    float tileSize = Constants::FLOOR_TILE_SIZE;
    
    // 计算房间左上角的瓦片坐标
    float baseX = bossCenter.x - tileSize * (roomTilesW / 2.0f - 0.5f);
    float baseY = bossCenter.y + tileSize * (roomTilesH / 2.0f - 0.5f);
    
    // 收集可用位置
    std::vector<std::pair<int, int>> availablePositions;
    int centerTileX = roomTilesW / 2;
    int centerTileY = roomTilesH / 2;
    
    for (int y = FIRE_EDGE_MARGIN; y < roomTilesH - FIRE_EDGE_MARGIN; ++y) {
        for (int x = FIRE_EDGE_MARGIN; x < roomTilesW - FIRE_EDGE_MARGIN; ++x) {
            // 排除Boss中心区域
            int dx = x - centerTileX;
            int dy = y - centerTileY;
            if (dx * dx + dy * dy <= FIRE_EXCLUSION_RADIUS * FIRE_EXCLUSION_RADIUS) {
                continue;
            }
            availablePositions.push_back({x, y});
        }
    }
    
    // 随机打乱位置
    for (size_t i = availablePositions.size() - 1; i > 0; --i) {
        size_t j = rand() % (i + 1);
        std::swap(availablePositions[i], availablePositions[j]);
    }
    
    // 生成火焰地板
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
    
    log("Generated %d fire floor decorations in boss room", fireCount);
}

/**
 * 生成连接走廊
 * 
 * 在起始房间和Boss房间之间生成水平走廊
 * 如果两个房间紧邻（无间隙），则不生成走廊
 */
void BossFloor::generateHallway(std::vector<Hallway*>& hallways) {
    if (!_startRoom || !_bossRoom) return;
    
    float tileSize = Constants::FLOOR_TILE_SIZE;
    float startRoomWidth = Constants::ROOM_TILES_W * tileSize;
    float bossRoomWidth = Constants::ROOM_TILES_W * 2 * tileSize;
    
    Vec2 startCenter = _startRoom->getCenter();
    Vec2 bossCenter = _bossRoom->getCenter();
    
    // 计算走廊范围
    float hallwayStartX = startCenter.x + startRoomWidth / 2;
    float hallwayEndX = bossCenter.x - bossRoomWidth / 2;
    float gapSize = hallwayEndX - hallwayStartX;
    
    // 只有当存在间隙时才生成走廊
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
}
