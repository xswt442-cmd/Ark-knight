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
    _chest = nullptr;  // 初始化宝箱指针
    _chestOpened = false;  // 初始化宝箱状态
    _portal = nullptr;  // 初始化传送门指针
    _portalLighting = nullptr;  // 初始化传送门闪电特效指针
    
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
        case Constants::RoomType::REWARD:
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

void Room::addSpikeAtPosition(const Vec2& pos)
{
    auto spike = Spike::create();
    if (!spike)
    {
        GAME_LOG("Failed to create spike sprite");
        return;
    }
    spike->setPosition(pos);
    spike->setGlobalZOrder(Constants::ZOrder::FLOOR + 1);
    this->addChild(spike, Constants::ZOrder::FLOOR + 1);
    _spikes.pushBack(spike);
}

void Room::addSpikeAtTile(int tileX, int tileY)
{
    float tileSize = Constants::FLOOR_TILE_SIZE;
    float startX = _centerX - tileSize * (_tilesWidth / 2.0f - 0.5f);
    float startY = _centerY + tileSize * (_tilesHeight / 2.0f - 0.5f);
    
    float spikeX = startX + tileX * tileSize;
    float spikeY = startY - tileY * tileSize;
    
    addSpikeAtPosition(Vec2(spikeX, spikeY));
}

void Room::addBoxAtPosition(const Vec2& pos, Box::BoxType type)
{
    auto box = Box::create(type);
    if (!box)
    {
        GAME_LOG("Failed to create box sprite");
        return;
    }
    box->setPosition(pos);
    box->setGlobalZOrder(Constants::ZOrder::WALL_ABOVE);
    this->addChild(box, Constants::ZOrder::WALL_ABOVE);
    _barriers.pushBack(box);
}

void Room::addBoxAtTile(int tileX, int tileY, Box::BoxType type)
{
    float tileSize = Constants::FLOOR_TILE_SIZE;
    float startX = _centerX - tileSize * (_tilesWidth / 2.0f - 0.5f);
    float startY = _centerY + tileSize * (_tilesHeight / 2.0f - 0.5f);
    
    float boxX = startX + tileX * tileSize;
    float boxY = startY - tileY * tileSize;
    
    addBoxAtPosition(Vec2(boxX, boxY), type);
}

void Room::addPillarAtPosition(const Vec2& pos, Pillar::PillarType type)
{
    auto pillar = Pillar::create(type);
    if (!pillar)
    {
        GAME_LOG("Failed to create pillar sprite");
        return;
    }
    pillar->setPosition(pos);
    pillar->setGlobalZOrder(Constants::ZOrder::WALL_ABOVE);
    this->addChild(pillar, Constants::ZOrder::WALL_ABOVE);
    _barriers.pushBack(pillar);
}

void Room::addPillarAtTile(int tileX, int tileY, Pillar::PillarType type)
{
    float tileSize = Constants::FLOOR_TILE_SIZE;
    float startX = _centerX - tileSize * (_tilesWidth / 2.0f - 0.5f);
    float startY = _centerY + tileSize * (_tilesHeight / 2.0f - 0.5f);
    
    float pillarX = startX + tileX * tileSize;
    float pillarY = startY - tileY * tileSize;
    
    addPillarAtPosition(Vec2(pillarX, pillarY), type);
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

// ==================== 地形布局系统 ====================

void Room::applyTerrainLayout(TerrainLayout layout)
{
    switch (layout)
    {
    case TerrainLayout::FIVE_BOXES:
        layoutFiveBoxes();
        break;
    case TerrainLayout::NINE_BOXES:
        layoutNineBoxes();
        break;
    case TerrainLayout::UPDOWN_SPIKES:
        layoutUpDownSpikes();
        break;
    case TerrainLayout::LEFTRIGHT_SPIKES:
        layoutLeftRightSpikes();
        break;
    case TerrainLayout::ALL_SPIKES:
        layoutAllSpikes();
        break;
    case TerrainLayout::UPDOWN_WALLS:
        layoutUpDownWalls();
        break;
    case TerrainLayout::LEFTRIGHT_WALLS:
        layoutLeftRightWalls();
        break;
    case TerrainLayout::CENTER_PILLAR:
        layoutCenterPillar();
        break;
    case TerrainLayout::FOUR_PILLARS:
        layoutFourPillars();
        break;
    case TerrainLayout::RANDOM_MESS:
        layoutRandomMess();
        break;
    case TerrainLayout::NONE:
    default:
        break;
    }
}

void Room::addBoxCluster3x3(int centerTileX, int centerTileY)
{
    // 随机选择一种木箱材质（单堆内统一）
    int typeIndex = cocos2d::RandomHelper::random_int(0, 2);
    Box::BoxType type;
    switch (typeIndex)
    {
    case 0: type = Box::BoxType::NORMAL; break;
    case 1: type = Box::BoxType::LIGHT; break;
    case 2: type = Box::BoxType::DARK; break;
    default: type = Box::BoxType::NORMAL; break;
    }
    
    // 放置3x3的木箱
    for (int dy = -1; dy <= 1; dy++)
    {
        for (int dx = -1; dx <= 1; dx++)
        {
            addBoxAtTile(centerTileX + dx, centerTileY + dy, type);
        }
    }
}

void Room::addPillarCluster2x2(int centerTileX, int centerTileY)
{
    // 随机选择一种石柱材质（单堆内统一）
    int typeIndex = cocos2d::RandomHelper::random_int(0, 2);
    Pillar::PillarType type;
    switch (typeIndex)
    {
    case 0: type = Pillar::PillarType::CLEAR; break;
    case 1: type = Pillar::PillarType::BROKEN; break;
    case 2: type = Pillar::PillarType::GLASSES; break;
    default: type = Pillar::PillarType::CLEAR; break;
    }
    
    // 放置2x2的石柱（中心点偏移0.5，确保2x2居中）
    for (int dy = 0; dy <= 1; dy++)
    {
        for (int dx = 0; dx <= 1; dx++)
        {
            addPillarAtTile(centerTileX + dx, centerTileY + dy, type);
        }
    }
}

void Room::layoutFiveBoxes()
{
    // 离墙最近的一圈是禁止放置区，所以有效范围是 [2, _tilesWidth-3] x [2, _tilesHeight-3]
    // 房间大小：26x18（索引从0开始）
    // 有效范围：[2, 23] x [2, 15]
    
    int centerX = _tilesWidth / 2;
    int centerY = _tilesHeight / 2;
    
    // 计算5个位置（左上、左下、右上、右下、中心），四周更靠近边缘
    int leftX = 4;    // 更靠左
    int rightX = _tilesWidth - 5;
    int topY = _tilesHeight - 5;
    int bottomY = 4;
    
    // 左上
    addBoxCluster3x3(leftX, topY);
    // 左下
    addBoxCluster3x3(leftX, bottomY);
    // 右上
    addBoxCluster3x3(rightX, topY);
    // 右下
    addBoxCluster3x3(rightX, bottomY);
    // 中心
    addBoxCluster3x3(centerX, centerY);
}

void Room::layoutNineBoxes()
{
    // 先放5堆
    layoutFiveBoxes();
    
    // 再放4堆（左中、上中、右中、下中）
    int centerX = _tilesWidth / 2;
    int centerY = _tilesHeight / 2;
    int leftX = 4;
    int rightX = _tilesWidth - 5;
    int topY = _tilesHeight - 5;
    int bottomY = 4;
    
    // 左中
    addBoxCluster3x3(leftX, centerY);
    // 上中
    addBoxCluster3x3(centerX, topY);
    // 右中
    addBoxCluster3x3(rightX, centerY);
    // 下中
    addBoxCluster3x3(centerX, bottomY);
}

void Room::layoutUpDownSpikes()
{
    // 矩形围城：左右墙，上下地刺，四角墙
    // 在有效范围内创建一个小矩形，更靠边缘
    int innerLeft = 4;
    int innerRight = _tilesWidth - 5;
    int innerTop = _tilesHeight - 5;
    int innerBottom = 4;
    
    // 左右墙（宽度1）
    for (int y = innerBottom; y <= innerTop; y++)
    {
        addBoxAtTile(innerLeft, y, Box::BoxType::NORMAL);
        addBoxAtTile(innerRight, y, Box::BoxType::NORMAL);
    }
    
    // 上下地刺（宽度1）
    for (int x = innerLeft + 1; x < innerRight; x++)
    {
        addSpikeAtTile(x, innerTop);
        addSpikeAtTile(x, innerBottom);
    }
}

void Room::layoutLeftRightSpikes()
{
    // 矩形围城：上下墙，左右地刺，四角墙
    int innerLeft = 4;
    int innerRight = _tilesWidth - 5;
    int innerTop = _tilesHeight - 5;
    int innerBottom = 4;
    
    // 上下墙（宽度1）
    for (int x = innerLeft; x <= innerRight; x++)
    {
        addBoxAtTile(x, innerTop, Box::BoxType::NORMAL);
        addBoxAtTile(x, innerBottom, Box::BoxType::NORMAL);
    }
    
    // 左右地刺（宽度1）
    for (int y = innerBottom + 1; y < innerTop; y++)
    {
        addSpikeAtTile(innerLeft, y);
        addSpikeAtTile(innerRight, y);
    }
}

void Room::layoutAllSpikes()
{
    // 一圈地刺，四角也是地刺
    int innerLeft = 4;
    int innerRight = _tilesWidth - 5;
    int innerTop = _tilesHeight - 5;
    int innerBottom = 4;
    
    // 上下地刺
    for (int x = innerLeft; x <= innerRight; x++)
    {
        addSpikeAtTile(x, innerTop);
        addSpikeAtTile(x, innerBottom);
    }
    
    // 左右地刺
    for (int y = innerBottom + 1; y < innerTop; y++)
    {
        addSpikeAtTile(innerLeft, y);
        addSpikeAtTile(innerRight, y);
    }
}

void Room::layoutUpDownWalls()
{
    // 上下各一排墙，更靠边缘
    int wallY_top = _tilesHeight - 4;
    int wallY_bottom = 3;
    
    for (int x = 2; x < _tilesWidth - 2; x++)
    {
        addBoxAtTile(x, wallY_top, Box::BoxType::NORMAL);
        addBoxAtTile(x, wallY_bottom, Box::BoxType::NORMAL);
    }
}

void Room::layoutLeftRightWalls()
{
    // 左右各一排墙，更靠边缘
    int wallX_left = 3;
    int wallX_right = _tilesWidth - 4;
    
    for (int y = 2; y < _tilesHeight - 2; y++)
    {
        addBoxAtTile(wallX_left, y, Box::BoxType::NORMAL);
        addBoxAtTile(wallX_right, y, Box::BoxType::NORMAL);
    }
}

void Room::layoutCenterPillar()
{
    // 中心2x2石柱
    int centerX = _tilesWidth / 2 - 1;  // 偏移以放置2x2
    int centerY = _tilesHeight / 2 - 1;
    
    addPillarCluster2x2(centerX, centerY);
}

void Room::layoutFourPillars()
{
    // 四个角各放2x2石柱，更靠边缘
    int leftX = 3;
    int rightX = _tilesWidth - 5;  // 预留2x2空间
    int topY = _tilesHeight - 5;
    int bottomY = 3;
    
    // 左上
    addPillarCluster2x2(leftX, topY);
    // 左下
    addPillarCluster2x2(leftX, bottomY);
    // 右上
    addPillarCluster2x2(rightX, topY);
    // 右下
    addPillarCluster2x2(rightX, bottomY);
}

void Room::layoutRandomMess()
{
    // 在禁止放置区之外随机放置10个石柱和10个木箱
    // 有效范围：[2, _tilesWidth-3] x [2, _tilesHeight-3]
    
    // 放置10个石柱
    for (int i = 0; i < 10; i++)
    {
        int x = cocos2d::RandomHelper::random_int(2, _tilesWidth - 3);
        int y = cocos2d::RandomHelper::random_int(2, _tilesHeight - 3);
        
        // 随机选择石柱类型
        int typeIndex = cocos2d::RandomHelper::random_int(0, 2);
        Pillar::PillarType type;
        switch (typeIndex)
        {
        case 0: type = Pillar::PillarType::CLEAR; break;
        case 1: type = Pillar::PillarType::BROKEN; break;
        case 2: type = Pillar::PillarType::GLASSES; break;
        default: type = Pillar::PillarType::CLEAR; break;
        }
        
        addPillarAtTile(x, y, type);
    }
    
    // 放置10个木箱
    for (int i = 0; i < 10; i++)
    {
        int x = cocos2d::RandomHelper::random_int(2, _tilesWidth - 3);
        int y = cocos2d::RandomHelper::random_int(2, _tilesHeight - 3);
        
        // 随机选择木箱类型
        int typeIndex = cocos2d::RandomHelper::random_int(0, 2);
        Box::BoxType type;
        switch (typeIndex)
        {
        case 0: type = Box::BoxType::NORMAL; break;
        case 1: type = Box::BoxType::LIGHT; break;
        case 2: type = Box::BoxType::DARK; break;
        default: type = Box::BoxType::NORMAL; break;
        }
        
        addBoxAtTile(x, y, type);
    }
}

// ==================== 宝箱生成 ====================

void Room::createChest()
{
    // 只在奖励房间生成宝箱
    if (_roomType != Constants::RoomType::REWARD)
    {
        return;
    }
    
    // 随机选择宝箱类型
    std::string chestPath;
    if (cocos2d::RandomHelper::random_int(0, 1) == 0)
    {
        chestPath = "Map/Chest/Wooden_chest.png";
    }
    else
    {
        chestPath = "Map/Chest/Iron_chest.png";
    }
    
    _chest = cocos2d::Sprite::create(chestPath);
    if (!_chest)
    {
        GAME_LOG("Failed to create chest sprite");
        return;
    }
    
    // 放置在房间中央
    _chest->setPosition(cocos2d::Vec2(_centerX, _centerY));
    
    // 缩放到合适大小（2倍地板砖大小）
    float targetSize = Constants::FLOOR_TILE_SIZE * 2.0f;
    float scale = targetSize / _chest->getContentSize().width;
    _chest->setScale(scale);
    
    _chest->setGlobalZOrder(Constants::ZOrder::FLOOR + 2);
    this->addChild(_chest, Constants::ZOrder::FLOOR + 2);
    
    GAME_LOG("Created chest at center of reward room");
}

bool Room::canInteractWithChest(Player* player) const
{
    if (!_chest || _chestOpened || !player)
    {
        return false;
    }
    
    // 检测玩家与宝箱的距离
    float interactionDistance = Constants::FLOOR_TILE_SIZE * 2.0f;  // 2格距离内可交互
    float distance = _chest->getPosition().distance(player->getPosition());
    
    return distance <= interactionDistance;
}

void Room::openChest()
{
    if (!_chest || _chestOpened)
    {
        return;
    }
    
    _chestOpened = true;
    
    // 播放打开动画：宝箱慢慢消失（淡出+缩小）
    auto fadeOut = cocos2d::FadeOut::create(0.5f);
    auto scaleDown = cocos2d::ScaleTo::create(0.5f, 0.0f);
    auto spawn = cocos2d::Spawn::create(fadeOut, scaleDown, nullptr);
    auto remove = cocos2d::RemoveSelf::create();
    auto sequence = cocos2d::Sequence::create(spawn, remove, nullptr);
    
    _chest->runAction(sequence);
    
    GAME_LOG("Chest opened!");
    
    // TODO: 这里之后添加奖励逻辑（武器、道具等）
}

// ==================== 传送门生成 ====================

void Room::createPortal()
{
    // 只在终点房间生成传送门
    if (_roomType != Constants::RoomType::END)
    {
        return;
    }
    
    // 创建传送门主体动画（7帧）
    cocos2d::Vector<cocos2d::SpriteFrame*> portalFrames;
    for (int i = 1; i <= 7; i++)
    {
        std::string framePath = "Map/Portal/Portal_000" + std::to_string(i) + ".png";
        auto frame = cocos2d::SpriteFrame::create(framePath, cocos2d::Rect(0, 0, 0, 0));
        if (frame)
        {
            portalFrames.pushBack(frame);
        }
    }
    
    if (portalFrames.empty())
    {
        GAME_LOG("Failed to load portal frames");
        return;
    }
    
    _portal = cocos2d::Sprite::createWithSpriteFrame(portalFrames.at(0));
    if (!_portal)
    {
        GAME_LOG("Failed to create portal sprite");
        return;
    }
    
    // 放置在房间中央
    _portal->setPosition(cocos2d::Vec2(_centerX, _centerY));
    
    // 缩放到合适大小（3倍地板砖大小）
    float targetSize = Constants::FLOOR_TILE_SIZE * 3.0f;
    float scale = targetSize / _portal->getContentSize().width;
    _portal->setScale(scale);
    
    _portal->setGlobalZOrder(Constants::ZOrder::FLOOR + 2);
    this->addChild(_portal, Constants::ZOrder::FLOOR + 2);
    
    // 播放传送门主体动画（循环）
    auto portalAnimation = cocos2d::Animation::createWithSpriteFrames(portalFrames, 0.1f);
    auto portalAnimate = cocos2d::Animate::create(portalAnimation);
    auto portalRepeat = cocos2d::RepeatForever::create(portalAnimate);
    _portal->runAction(portalRepeat);
    
    // 创建闪电特效动画（4帧）
    cocos2d::Vector<cocos2d::SpriteFrame*> lightingFrames;
    for (int i = 1; i <= 4; i++)
    {
        std::string framePath = "Map/Portal/Portallighting_000" + std::to_string(i) + ".png";
        auto frame = cocos2d::SpriteFrame::create(framePath, cocos2d::Rect(0, 0, 0, 0));
        if (frame)
        {
            lightingFrames.pushBack(frame);
        }
    }
    
    if (!lightingFrames.empty())
    {
        _portalLighting = cocos2d::Sprite::createWithSpriteFrame(lightingFrames.at(0));
        if (_portalLighting)
        {
            // 闪电特效放在传送门同一位置
            _portalLighting->setPosition(cocos2d::Vec2(_centerX, _centerY));
            _portalLighting->setScale(scale);
            _portalLighting->setGlobalZOrder(Constants::ZOrder::FLOOR + 3);
            this->addChild(_portalLighting, Constants::ZOrder::FLOOR + 3);
            
            // 播放闪电动画（循环，速度更快）
            auto lightingAnimation = cocos2d::Animation::createWithSpriteFrames(lightingFrames, 0.08f);
            auto lightingAnimate = cocos2d::Animate::create(lightingAnimation);
            auto lightingRepeat = cocos2d::RepeatForever::create(lightingAnimate);
            _portalLighting->runAction(lightingRepeat);
        }
    }
    
    GAME_LOG("Created portal at center of end room");
}

bool Room::canInteractWithPortal(Player* player) const
{
    if (!_portal || !player)
    {
        return false;
    }
    
    // 检测玩家与传送门的距离
    float interactionDistance = Constants::FLOOR_TILE_SIZE * 2.5f;
    float distance = _portal->getPosition().distance(player->getPosition());
    
    return distance <= interactionDistance;
}
