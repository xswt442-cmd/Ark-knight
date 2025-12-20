#include "MiniMap.h"
#include "Map/MapGenerator.h"
#include "Map/Room.h"

USING_NS_CC;

using namespace Constants;

// ==================== MiniRoom Implementation ====================

MiniRoom* MiniRoom::create() {
    MiniRoom* room = new (std::nothrow) MiniRoom();
    if (room && room->init()) {
        room->autorelease();
        return room;
    }
    CC_SAFE_DELETE(room);
    return nullptr;
}

bool MiniRoom::init() {
    if (!Node::init()) {
        return false;
    }
    
    _isCurrent = false;
    
    // 创建背景绘制节点
    _background = DrawNode::create();
    this->addChild(_background);
    
    // 创建四个方向的门绘制节点
    for (int i = 0; i < 4; i++) {
        _doors[i] = DrawNode::create();
        _doors[i]->setVisible(false);
        this->addChild(_doors[i]);
    }
    
    return true;
}

void MiniRoom::setRoomColor(RoomType type) {
    Color4F color;
    
    switch (type) {
        case RoomType::BEGIN:
            color = Color4F(0.0f, 0.9f, 0.0f, 0.6f);  // 绿色
            break;
        case RoomType::END:
            color = Color4F(0.0f, 0.0f, 1.0f, 0.7f);  // 蓝色
            break;
        case RoomType::BOSS:
            color = Color4F(0.9f, 0.0f, 0.0f, 0.6f);  // 红色
            break;
        case RoomType::WEAPON:
            color = Color4F(0.8f, 0.5f, 0.0f, 0.6f);  // 橙色
            break;
        case RoomType::PROP:
            color = Color4F(1.0f, 0.8f, 0.0f, 0.6f);  // 黄色
            break;
        case RoomType::NORMAL:
        default:
            color = Color4F(0.3f, 0.3f, 0.3f, 0.6f);  // 灰色
            break;
    }
    
    float size = 16.0f;  // 房间大小
    _background->clear();
    _background->drawSolidRect(Vec2(-size/2, -size/2), Vec2(size/2, size/2), color);
    
    // 边框
    Color4F borderColor = _isCurrent ? Color4F::WHITE : Color4F(0.5f, 0.5f, 0.5f, 0.8f);
    float borderWidth = _isCurrent ? 2.0f : 1.0f;
    _background->drawRect(Vec2(-size/2, -size/2), Vec2(size/2, size/2), borderColor);
}

void MiniRoom::setCurrent(bool isCurrent) {
    _isCurrent = isCurrent;
    
    // 重绘边框
    float size = 16.0f;
    Color4F borderColor = _isCurrent ? Color4F::WHITE : Color4F(0.5f, 0.5f, 0.5f, 0.8f);
    
    // 不清空背景，只更新边框（需要重新设置颜色）
}

void MiniRoom::setVisited(bool visited) {
    if (visited) {
        // 已访问的房间变暗
        this->setOpacity(128);
    } else {
        this->setOpacity(255);
    }
}

void MiniRoom::setDoorVisible(int direction, bool visible) {
    if (direction >= 0 && direction < 4) {
        _doors[direction]->setVisible(visible);
        
        if (visible) {
            // 绘制门连接线
            _doors[direction]->clear();
            
            float lineLength = 6.0f;
            Vec2 start, end;
            
            switch (direction) {
                case Direction::UP:
                    start = Vec2(0, 8);
                    end = Vec2(0, 8 + lineLength);
                    break;
                case Direction::RIGHT:
                    start = Vec2(8, 0);
                    end = Vec2(8 + lineLength, 0);
                    break;
                case Direction::DOWN:
                    start = Vec2(0, -8);
                    end = Vec2(0, -8 - lineLength);
                    break;
                case Direction::LEFT:
                    start = Vec2(-8, 0);
                    end = Vec2(-8 - lineLength, 0);
                    break;
            }
            
            _doors[direction]->drawLine(start, end, Color4F(0.6f, 0.6f, 0.6f, 0.8f));
        }
    }
}

// ==================== MiniMap Implementation ====================

MiniMap* MiniMap::create() {
    MiniMap* miniMap = new (std::nothrow) MiniMap();
    if (miniMap && miniMap->init()) {
        miniMap->autorelease();
        return miniMap;
    }
    CC_SAFE_DELETE(miniMap);
    return nullptr;
}

bool MiniMap::init() {
    if (!Node::init()) {
        return false;
    }
    
    // 初始化小地图配置
    _roomSize = 16.0f;
    _gap = 6.0f;
    _totalWidth = Map::MAP_SIZE * (_roomSize + _gap);
    _totalHeight = Map::MAP_SIZE * (_roomSize + _gap);
    
    _currentRoom = nullptr;
    _currentGridX = -1;
    _currentGridY = -1;
    
    // 初始化矩阵为nullptr
    for (int y = 0; y < Map::MAP_SIZE; y++) {
        for (int x = 0; x < Map::MAP_SIZE; x++) {
            _miniRooms[x][y] = nullptr;
        }
    }
    
    // 创建背景
    auto background = DrawNode::create();
    float bgPadding = 10.0f;
    background->drawSolidRect(
        Vec2(-bgPadding, -bgPadding),
        Vec2(_totalWidth + bgPadding, _totalHeight + bgPadding),
        Color4F(0.0f, 0.0f, 0.0f, 0.3f)
    );
    this->addChild(background, -1);
    
    // 设置小地图位置（右上角）
    Size visibleSize = Director::getInstance()->getVisibleSize();
    this->setPosition(Vec2(
        visibleSize.width - _totalWidth - 20,
        visibleSize.height - _totalHeight - 20
    ));
    
    // 设置Z序
    this->setGlobalZOrder(ZOrder::MINIMAP);
    
    return true;
}

void MiniMap::initFromMapGenerator(MapGenerator* generator) {
    if (!generator) return;
    
    // 清理旧的小地图房间
    for (int y = 0; y < Map::MAP_SIZE; y++) {
        for (int x = 0; x < Map::MAP_SIZE; x++) {
            if (_miniRooms[x][y]) {
                _miniRooms[x][y]->removeFromParent();
                _miniRooms[x][y] = nullptr;
            }
        }
    }
    
    // 根据地图生成器创建小地图房间
    for (int y = 0; y < Map::MAP_SIZE; y++) {
        for (int x = 0; x < Map::MAP_SIZE; x++) {
            Room* room = generator->getRoom(x, y);
            if (room) {
                MiniRoom* miniRoom = MiniRoom::create();
                
                // 计算位置（y轴翻转，使小地图上方为北）
                float posX = x * (_roomSize + _gap) + _roomSize / 2;
                float posY = (Map::MAP_SIZE - 1 - y) * (_roomSize + _gap) + _roomSize / 2;
                miniRoom->setPosition(Vec2(posX, posY));
                
                // 设置房间类型颜色
                miniRoom->setRoomColor(room->getRoomType());
                
                // 设置门连接
                for (int dir = 0; dir < Direction::COUNT; dir++) {
                    miniRoom->setDoorVisible(dir, room->hasDoor(dir));
                }
                
                // 未访问的房间先隐藏（除了起始房间）
                if (room->getRoomType() != RoomType::BEGIN && !room->isVisited()) {
                    miniRoom->setVisible(false);
                }
                
                this->addChild(miniRoom);
                _miniRooms[x][y] = miniRoom;
            }
        }
    }
    
    // 设置起始房间为当前房间
    Room* beginRoom = generator->getBeginRoom();
    if (beginRoom) {
        updateCurrentRoom(beginRoom);
    }
}

void MiniMap::updateCurrentRoom(Room* currentRoom) {
    if (!currentRoom) return;
    
    // 取消之前的高亮
    if (_currentGridX >= 0 && _currentGridY >= 0) {
        MiniRoom* prevMiniRoom = _miniRooms[_currentGridX][_currentGridY];
        if (prevMiniRoom) {
            prevMiniRoom->setCurrent(false);
            prevMiniRoom->setVisited(true);
        }
    }
    
    _currentRoom = currentRoom;
    _currentGridX = currentRoom->getGridX();
    _currentGridY = currentRoom->getGridY();
    
    // 设置新的高亮
    MiniRoom* miniRoom = _miniRooms[_currentGridX][_currentGridY];
    if (miniRoom) {
        miniRoom->setVisible(true);
        miniRoom->setCurrent(true);
        miniRoom->setRoomColor(currentRoom->getRoomType());
        
        // 显示相邻房间
        for (int dir = 0; dir < Direction::COUNT; dir++) {
            if (currentRoom->hasDoor(dir)) {
                int adjX = _currentGridX + Direction::DX[dir];
                int adjY = _currentGridY + Direction::DY[dir];
                
                if (adjX >= 0 && adjX < Map::MAP_SIZE &&
                    adjY >= 0 && adjY < Map::MAP_SIZE) {
                    MiniRoom* adjMiniRoom = _miniRooms[adjX][adjY];
                    if (adjMiniRoom) {
                        adjMiniRoom->setVisible(true);
                    }
                }
            }
        }
    }
}

void MiniMap::updateRoomVisited(int gridX, int gridY) {
    if (gridX < 0 || gridX >= Map::MAP_SIZE ||
        gridY < 0 || gridY >= Map::MAP_SIZE) {
        return;
    }
    
    MiniRoom* miniRoom = _miniRooms[gridX][gridY];
    if (miniRoom) {
        miniRoom->setVisible(true);
        miniRoom->setVisited(true);
    }
}

MiniRoom* MiniMap::getMiniRoom(int x, int y) {
    if (x < 0 || x >= Map::MAP_SIZE || y < 0 || y >= Map::MAP_SIZE) {
        return nullptr;
    }
    return _miniRooms[x][y];
}
