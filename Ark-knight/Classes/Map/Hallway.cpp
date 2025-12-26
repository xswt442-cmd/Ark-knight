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
    
    // 默认瓦片数，会在setGapSize中根据实际空隙重新计算
    if (_direction == Constants::DIR_UP || _direction == Constants::DIR_DOWN) {
        // 垂直走廊：宽度 = 门宽（不含墙，与房间地板无缝衔接）
        _tilesWidth = Constants::DOOR_WIDTH;
        _tilesHeight = 12;  // 默认值，会被覆盖
    } else {
        // 水平走廊：高度 = 门宽（不含墙，与房间地板无缝衔接）
        _tilesWidth = 6;  // 默认值，会被覆盖
        _tilesHeight = Constants::DOOR_WIDTH;
    }
    
    return true;
}

void Hallway::setGapSize(float gapSize) {
    float tileSize = Constants::FLOOR_TILE_SIZE;
    // 根据实际空隙大小计算瓦片数，恰好填充两个房间之间的间隙
    int tilesNeeded = static_cast<int>(std::ceil(gapSize / tileSize));
    
    if (tilesNeeded < 1) tilesNeeded = 1;
    
    if (_direction == Constants::DIR_UP || _direction == Constants::DIR_DOWN) {
        _tilesHeight = tilesNeeded;
    } else {
        _tilesWidth = tilesNeeded;
    }
}

void Hallway::setCenter(float x, float y) {
    _centerX = x;
    _centerY = y;
}

void Hallway::createMap() {
    float tileSize = Constants::FLOOR_TILE_SIZE;
    // 对于偶数瓦片，中心在两个瓦片之间
    float startX = _centerX - tileSize * (_tilesWidth / 2.0f - 0.5f);
    float startY = _centerY + tileSize * (_tilesHeight / 2.0f - 0.5f);
    
    // 生成走廊地板（不生成墙体，与房间地板无缝衔接）
    for (int h = 0; h < _tilesHeight; h++) {
        for (int w = 0; w < _tilesWidth; w++) {
            float x = startX + w * tileSize;
            float y = startY - h * tileSize;
            
            // 走廊全部使用地板，不生成墙体
            generateFloor(x, y);
        }
    }
    
    // 更新实际可行走边界（走廊全为地板，边界即为走廊范围）
    float playerHalfSize = 15.0f;  // 玩家半径约15像素
    
    if (_direction == Constants::DIR_LEFT || _direction == Constants::DIR_RIGHT) {
        // 水平走廊：全为地板，上下边界为地板边缘
        _leftX = startX;
        _rightX = startX + tileSize * (_tilesWidth - 1);
        _topY = startY + tileSize / 2 - playerHalfSize;
        _bottomY = startY - (_tilesHeight - 1) * tileSize - tileSize / 2 + playerHalfSize;
    } else {
        // 垂直走廊：全为地板，左右边界为地板边缘
        _leftX = startX - tileSize / 2 + playerHalfSize;
        _rightX = startX + (_tilesWidth - 1) * tileSize + tileSize / 2 - playerHalfSize;
        _topY = startY;
        _bottomY = startY - tileSize * (_tilesHeight - 1);
    }
    
    log("Hallway dir=%d center=(%.1f,%.1f) tiles=%dx%d walkable: X[%.1f,%.1f] Y[%.1f,%.1f]",
        _direction, _centerX, _centerY, _tilesWidth, _tilesHeight, _leftX, _rightX, _bottomY, _topY);
}

void Hallway::generateFloor(float x, float y) {
    // 走廊使用专用地板纹理Floor_cor
    auto floor = Sprite::create("Map/Floor/Floor_cor.png");
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

bool Hallway::checkPlayerPosition(Player* player, float& speedX, float& speedY) {
    if (!player) return false;
    
    // 直接使用玩家的本地坐标（相对于_gameLayer）
    Vec2 pos = player->getPosition();
    float tileSize = Constants::FLOOR_TILE_SIZE;
    
    // 根据走廊方向检测
    if (_direction == Constants::DIR_LEFT || _direction == Constants::DIR_RIGHT) {
        // 水平走廊：扩展X方向检测范围，严格限制Y方向
        if (pos.x >= _leftX - tileSize && pos.x <= _rightX + tileSize &&
            pos.y <= _topY + tileSize + 30 && pos.y >= _bottomY - tileSize - 15) {
            
            // 限制Y轴（不能穿过上下墙）
            if (speedY > 0 && pos.y >= _topY + tileSize / 2) speedY = 0.0f;
            if (speedY < 0 && pos.y <= _bottomY) speedY = 0.0f;
            
            // 严格判断是否在走廊内部
            if (pos.x > _leftX - tileSize && pos.x < _rightX + tileSize &&
                pos.y < _topY + tileSize && pos.y > _bottomY - tileSize) {
                return true;
            }
        }
    }
    else {
        // 垂直走廊：扩展Y方向检测范围，严格限制X方向
        if (pos.x >= _leftX - tileSize - 30 && pos.x <= _rightX + tileSize + 30 &&
            pos.y <= _topY + tileSize && pos.y >= _bottomY - tileSize) {
            
            // 限制X轴（不能穿过左右墙）
            if (speedX > 0 && pos.x >= _rightX) speedX = 0.0f;
            if (speedX < 0 && pos.x <= _leftX) speedX = 0.0f;
            
            // 严格判断是否在走廊内部
            if (pos.x > _leftX - tileSize && pos.x < _rightX + tileSize &&
                pos.y < _topY + tileSize && pos.y > _bottomY - tileSize) {
                return true;
            }
        }
    }
    
    return false;
}

bool Hallway::isPlayerInHallway(Player* player) const {
    if (!player) return false;
    
    // 直接使用玩家的本地坐标
    Vec2 pos = player->getPosition();
    
    // 严格检测是否在走廊区域内
    return (pos.x >= _leftX && pos.x <= _rightX &&
            pos.y >= _bottomY && pos.y <= _topY);
}
