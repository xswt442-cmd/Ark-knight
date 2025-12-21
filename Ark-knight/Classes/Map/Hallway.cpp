#include "Hallway.h"
#include "Entities/Player/Player.h"
#include <cmath>

USING_NS_CC;

Hallway* Hallway::create(int direction) {
    Hallway* hallway = new (std::nothrow) Hallway();
    if (hallway && hallway->initWithDirection(direction)) {
        hallway->autorelease();
        return hallway;
    }
    CC_SAFE_DELETE(hallway);
    return nullptr;
}

bool Hallway::init() {
    if (!Node::init()) {
        return false;
    }
    return true;
}

bool Hallway::initWithDirection(int direction) {
    if (!init()) {
        return false;
    }
    
    _direction = direction;
    _centerX = 0.0f;
    _centerY = 0.0f;
    
    // 走廊只填充房间之间的空隙，不覆盖房间内部
    // 水平空隙 = 900 - 800 = 100px ≈ 3瓦片
    // 垂直空隙 = 900 - 544 = 356px ≈ 11瓦片
    float tileSize = Constants::FLOOR_TILE_SIZE;
    
    if (_direction == Constants::DIR_UP || _direction == Constants::DIR_DOWN) {
        // 垂直走廊
        _tilesWidth = Constants::DOOR_WIDTH;
        float verticalGap = Constants::ROOM_CENTER_DIST - Constants::ROOM_TILES_H * tileSize;
        _tilesHeight = static_cast<int>(std::ceil(verticalGap / tileSize));
    } else {
        // 水平走廊
        float horizontalGap = Constants::ROOM_CENTER_DIST - Constants::ROOM_TILES_W * tileSize;
        _tilesWidth = static_cast<int>(std::ceil(horizontalGap / tileSize));
        _tilesHeight = Constants::DOOR_WIDTH;
    }
    
    return true;
}

void Hallway::setCenter(float x, float y) {
    _centerX = x;
    _centerY = y;
}

void Hallway::createMap() {
    float tileSize = Constants::FLOOR_TILE_SIZE;
    float startX = _centerX - tileSize * (_tilesWidth / 2.0f);
    float startY = _centerY + tileSize * (_tilesHeight / 2.0f);
    
    // 计算边界
    _leftX = startX;
    _rightX = startX + tileSize * _tilesWidth;
    _topY = startY;
    _bottomY = startY - tileSize * _tilesHeight;
    
    // 走廊只生成地板
    for (int h = 0; h < _tilesHeight; h++) {
        for (int w = 0; w < _tilesWidth; w++) {
            float x = startX + w * tileSize;
            float y = startY - h * tileSize;
            generateFloor(x, y);
        }
    }
}

void Hallway::generateFloor(float x, float y) {
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

bool Hallway::checkPlayerPosition(Player* player, float& speedX, float& speedY) {
    if (!player) return false;
    
    Vec2 pos = player->getPosition();
    float tileSize = Constants::FLOOR_TILE_SIZE;
    
    // 根据走廊方向限制玩家移动
    if (_direction == Constants::DIR_LEFT || _direction == Constants::DIR_RIGHT) {
        // 水平走廊：只限制Y轴，X轴可以自由通过
        if (pos.x >= _leftX - tileSize && pos.x <= _rightX + tileSize &&
            pos.y <= _topY + tileSize && pos.y >= _bottomY - tileSize) {
            
            // 限制Y轴不能超出走廊宽度
            if (speedY > 0 && pos.y >= _topY + tileSize / 2) {
                speedY = 0.0f;
            }
            else if (speedY < 0 && pos.y <= _bottomY) {
                speedY = 0.0f;
            }
            return true;
        }
    }
    else {
        // 垂直走廊：只限制X轴，Y轴可以自由通过
        if (pos.x >= _leftX - tileSize && pos.x <= _rightX + tileSize &&
            pos.y <= _topY + tileSize && pos.y >= _bottomY - tileSize) {
            
            // 限制X轴不能超出走廊宽度
            if (speedX > 0 && pos.x >= _rightX) {
                speedX = 0.0f;
            }
            else if (speedX < 0 && pos.x <= _leftX) {
                speedX = 0.0f;
            }
            return true;
        }
    }
    
    return false;
}

bool Hallway::isPlayerInHallway(Player* player) const {
    if (!player) return false;
    
    Vec2 pos = player->getPosition();
    float tileSize = Constants::FLOOR_TILE_SIZE;
    
    // 扩大一点范围来检测
    return (pos.x > _leftX - tileSize && pos.x < _rightX + tileSize &&
            pos.y < _topY + tileSize && pos.y > _bottomY - tileSize);
}
