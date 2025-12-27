#include "MiniMap.h"
#include "Map/MapGenerator.h"
#include "Map/Room.h"

USING_NS_CC;

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
    
    _background = DrawNode::create();
    _background->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL);
    this->addChild(_background);
    
    for (int i = 0; i < 4; i++) {
        _doors[i] = DrawNode::create();
        _doors[i]->setVisible(false);
        _doors[i]->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL);
        this->addChild(_doors[i]);
    }
    
    return true;
}

void MiniRoom::setRoomColor(Constants::RoomType type) {
    Color4F color;
    
    switch (type) {
        case Constants::RoomType::BEGIN:
            color = Color4F(0.0f, 0.8f, 0.0f, 0.8f);  // 绿色：起始房间
            break;
        case Constants::RoomType::END:
            color = Color4F(0.4f, 0.7f, 1.0f, 0.8f);  // 浅蓝色：传送门房间
            break;
        case Constants::RoomType::BOSS:
            color = Color4F(0.9f, 0.0f, 0.0f, 0.8f);  // 红色：BOSS房间
            break;
        case Constants::RoomType::REWARD:
            color = Color4F(0.9f, 0.6f, 0.1f, 0.8f);  // 橙色：奖励房间
            break;
        case Constants::RoomType::NORMAL:
        default:
            color = Color4F(0.3f, 0.3f, 0.3f, 0.8f);  // 灰色：普通战斗房间
            break;
    }
    
    float size = 16.0f;
    _background->clear();
    _background->drawSolidRect(Vec2(-size/2, -size/2), Vec2(size/2, size/2), color);
    
    Color4F borderColor = _isCurrent ? Color4F::WHITE : Color4F(0.5f, 0.5f, 0.5f, 0.8f);
    _background->drawRect(Vec2(-size/2, -size/2), Vec2(size/2, size/2), borderColor);
}

void MiniRoom::setCurrent(bool isCurrent) {
    _isCurrent = isCurrent;
}

void MiniRoom::setVisited(bool visited) {
    // 已访问的房间透明度降低，显示为变暗效果
    if (visited) {
        this->setOpacity(180);  // 透明度降低到70%
        _background->setOpacity(180);
        for (int i = 0; i < 4; i++) {
            _doors[i]->setOpacity(180);
        }
    } else {
        this->setOpacity(255);  // 未访问房间完全不透明
        _background->setOpacity(255);
        for (int i = 0; i < 4; i++) {
            _doors[i]->setOpacity(255);
        }
    }
}

void MiniRoom::setDoorVisible(int direction, bool visible) {
    if (direction >= 0 && direction < 4) {
        _doors[direction]->setVisible(visible);
        
        if (visible) {
            _doors[direction]->clear();
            
            float lineLength = 6.0f;
            Vec2 start, end;
            
            switch (direction) {
                case Constants::DIR_UP:
                    start = Vec2(0, 8);
                    end = Vec2(0, 8 + lineLength);
                    break;
                case Constants::DIR_RIGHT:
                    start = Vec2(8, 0);
                    end = Vec2(8 + lineLength, 0);
                    break;
                case Constants::DIR_DOWN:
                    start = Vec2(0, -8);
                    end = Vec2(0, -8 - lineLength);
                    break;
                case Constants::DIR_LEFT:
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
    
    _roomSize = 16.0f;
    _gap = 6.0f;
    _totalWidth = Constants::MAP_GRID_SIZE * (_roomSize + _gap);
    _totalHeight = Constants::MAP_GRID_SIZE * (_roomSize + _gap);
    
    _currentRoom = nullptr;
    _currentGridX = -1;
    _currentGridY = -1;
    
    for (int y = 0; y < Constants::MAP_GRID_SIZE; y++) {
        for (int x = 0; x < Constants::MAP_GRID_SIZE; x++) {
            _miniRooms[x][y] = nullptr;
        }
    }
    
    auto background = DrawNode::create();
    float bgPadding = 10.0f;
    background->drawSolidRect(
        Vec2(-bgPadding, -bgPadding),
        Vec2(_totalWidth + bgPadding, _totalHeight + bgPadding),
        Color4F(0.0f, 0.0f, 0.0f, 0.3f)
    );
    background->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL);
    this->addChild(background, -1);
    
    Size visibleSize = Director::getInstance()->getVisibleSize();
    this->setPosition(Vec2(
        visibleSize.width - _totalWidth - 20,
        visibleSize.height - _totalHeight - 20
    ));
    
    this->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL);
    
    // 创建关卡进度标签
    _levelLabel = Label::createWithTTF("1-1", "fonts/msyh.ttf", 18);
    _levelLabel->setTextColor(Color4B::WHITE);
    _levelLabel->enableOutline(Color4B::BLACK, 1);  // 添加黑色描边增强可读性
    _levelLabel->setPosition(Vec2(_totalWidth / 2, -30));  // 在小地图下方
    _levelLabel->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL);
    this->addChild(_levelLabel);
    
    return true;
}

void MiniMap::initFromMapGenerator(MapGenerator* generator) {
    if (!generator) return;
    
    for (int y = 0; y < Constants::MAP_GRID_SIZE; y++) {
        for (int x = 0; x < Constants::MAP_GRID_SIZE; x++) {
            if (_miniRooms[x][y]) {
                _miniRooms[x][y]->removeFromParent();
                _miniRooms[x][y] = nullptr;
            }
        }
    }
    
    for (int y = 0; y < Constants::MAP_GRID_SIZE; y++) {
        for (int x = 0; x < Constants::MAP_GRID_SIZE; x++) {
            Room* room = generator->getRoom(x, y);
            if (room) {
                MiniRoom* miniRoom = MiniRoom::create();
                
                // 计算位置（不翻转Y轴）
                float posX = x * (_roomSize + _gap) + _roomSize / 2;
                float posY = y * (_roomSize + _gap) + _roomSize / 2;
                miniRoom->setPosition(Vec2(posX, posY));
                
                miniRoom->setRoomColor(room->getRoomType());
                
                for (int dir = 0; dir < Constants::DIR_COUNT; dir++) {
                    miniRoom->setDoorVisible(dir, room->hasDoor(dir));
                }
                
                if (room->getRoomType() != Constants::RoomType::BEGIN && !room->isVisited()) {
                    miniRoom->setVisible(false);
                }
                
                this->addChild(miniRoom);
                _miniRooms[x][y] = miniRoom;
            }
        }
    }
    
    Room* beginRoom = generator->getBeginRoom();
    if (beginRoom) {
        updateCurrentRoom(beginRoom);
    }
}

void MiniMap::updateCurrentRoom(Room* currentRoom) {
    if (!currentRoom) return;
    
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
    
    MiniRoom* miniRoom = _miniRooms[_currentGridX][_currentGridY];
    if (miniRoom) {
        miniRoom->setVisible(true);
        miniRoom->setCurrent(true);
        miniRoom->setRoomColor(currentRoom->getRoomType());
        
        // 方向偏移数组
        static const int DIR_DX[] = {0, 1, 0, -1};
        static const int DIR_DY[] = {1, 0, -1, 0};
        
        for (int dir = 0; dir < Constants::DIR_COUNT; dir++) {
            if (currentRoom->hasDoor(dir)) {
                int adjX = _currentGridX + DIR_DX[dir];
                int adjY = _currentGridY + DIR_DY[dir];
                
                if (adjX >= 0 && adjX < Constants::MAP_GRID_SIZE &&
                    adjY >= 0 && adjY < Constants::MAP_GRID_SIZE) {
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
    if (gridX < 0 || gridX >= Constants::MAP_GRID_SIZE ||
        gridY < 0 || gridY >= Constants::MAP_GRID_SIZE) {
        return;
    }
    
    MiniRoom* miniRoom = _miniRooms[gridX][gridY];
    if (miniRoom) {
        miniRoom->setVisible(true);
        miniRoom->setVisited(true);
    }
}

MiniRoom* MiniMap::getMiniRoom(int x, int y) {
    if (x < 0 || x >= Constants::MAP_GRID_SIZE || 
        y < 0 || y >= Constants::MAP_GRID_SIZE) {
        return nullptr;
    }
    return _miniRooms[x][y];
}

void MiniMap::updateLevelDisplay(int level, int stage) {
    if (_levelLabel) {
        char levelText[16];
        // stage=0表示Boss层
        if (stage == 0) {
            sprintf(levelText, "Boss");
        } else {
            sprintf(levelText, "%d-%d", level, stage);
        }
        _levelLabel->setString(levelText);
    }
}
