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
    
    // 计算边界（包括墙壁）
    _leftX = startX;
    _rightX = startX + tileSize * _tilesWidth;
    _topY = startY;
    _bottomY = startY - tileSize * _tilesHeight;
    
    // 生成地板和墙壁
    for (int h = 0; h < _tilesHeight; h++) {
        for (int w = 0; w < _tilesWidth; w++) {
            float x = startX + w * tileSize;
            float y = startY - h * tileSize;
            
            bool isWall = false;
            int zOrder = Constants::ZOrder::WALL_ABOVE;
            
            if (_direction == Constants::DIR_LEFT || _direction == Constants::DIR_RIGHT) {
                // 水平走廊：上下两侧是墙
                if (h == 0) {
                    isWall = true;
                    zOrder = Constants::ZOrder::WALL_BELOW;
                } else if (h == _tilesHeight - 1) {
                    isWall = true;
                    zOrder = Constants::ZOrder::WALL_ABOVE;
                }
            } else {
                // 垂直走廊：左右两侧是墙
                if (w == 0 || w == _tilesWidth - 1) {
                    isWall = true;
                    zOrder = Constants::ZOrder::WALL_ABOVE;
                }
            }
            
            if (isWall) {
                generateWall(x, y, zOrder);
            } else {
                generateFloor(x, y);
            }
        }
    }
    
    // 更新实际可行走边界（排除墙壁）
    if (_direction == Constants::DIR_LEFT || _direction == Constants::DIR_RIGHT) {
        // 水平走廊：排除上下墙壁
        // 第1行是墙，可行走从第2行开始
        _topY = startY - tileSize;  // 顶部墙的下边缘
        _bottomY = startY - tileSize * (_tilesHeight - 1);  // 底部墙的上边缘
    } else {
        // 垂直走廊：排除左右墙壁
        _leftX = startX + tileSize;  // 左墙的右边缘
        _rightX = startX + tileSize * (_tilesWidth - 1);  // 右墙的左边缘
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

void Hallway::generateWall(float x, float y, int zOrder) {
    auto wall = Sprite::create("res/wall.png");
    if (!wall) {
        wall = Sprite::create();
        wall->setTextureRect(Rect(0, 0, Constants::FLOOR_TILE_SIZE, Constants::FLOOR_TILE_SIZE));
        wall->setColor(Color3B(80, 80, 100));
    }
    
    wall->setPosition(Vec2(x, y));
    wall->setGlobalZOrder(zOrder);
    this->addChild(wall, zOrder);
    _walls.pushBack(wall);
}

bool Hallway::checkPlayerPosition(Player* player, float& speedX, float& speedY) {
    if (!player) return false;
    
    Vec2 pos = player->getPosition();
    float tileSize = Constants::FLOOR_TILE_SIZE;
    float halfTile = tileSize / 2.0f;
    
    // 根据走廊方向限制玩家位置
    if (_direction == Constants::DIR_LEFT || _direction == Constants::DIR_RIGHT) {
        // 水平走廊：只限制Y轴（检测是否在走廊X范围和扩展的Y范围内）
        if (pos.x >= _leftX - halfTile && pos.x <= _rightX + halfTile &&
            pos.y <= _topY + tileSize && pos.y >= _bottomY - tileSize) {
            
            // 限制Y轴不能超出走廊可行走区域
            if (pos.y > _topY) {
                pos.y = _topY;
                player->setPosition(pos);
            }
            else if (pos.y < _bottomY) {
                pos.y = _bottomY;
                player->setPosition(pos);
            }
            return true;
        }
    }
    else {
        // 垂直走廊：只限制X轴（检测是否在走廊Y范围和扩展的X范围内）
        if (pos.x >= _leftX - tileSize && pos.x <= _rightX + tileSize &&
            pos.y <= _topY + halfTile && pos.y >= _bottomY - halfTile) {
            
            // 限制X轴不能超出走廊可行走区域
            if (pos.x > _rightX) {
                pos.x = _rightX;
                player->setPosition(pos);
            }
            else if (pos.x < _leftX) {
                pos.x = _leftX;
                player->setPosition(pos);
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
