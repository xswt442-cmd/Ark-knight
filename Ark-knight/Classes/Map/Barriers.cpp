#include "Barriers.h"
#include "Entities/Player/Player.h"

USING_NS_CC;

// ==================== Spike ====================

Spike* Spike::create(const std::string& texturePath)
{
    Spike* spike = new (std::nothrow) Spike();
    if (spike && spike->initWithTexturePath(texturePath))
    {
        spike->autorelease();
        return spike;
    }
    CC_SAFE_DELETE(spike);
    return nullptr;
}

bool Spike::initWithTexturePath(const std::string& texturePath)
{
    if (!Sprite::initWithFile(texturePath))
    {
        return false;
    }
    
    // 缩放到地砖大小
    float targetSize = Constants::FLOOR_TILE_SIZE;
    float scale = targetSize / this->getContentSize().width;
    this->setScale(scale);
    
    // 初始颜色稍微压暗，触发后会变白
    this->setColor(Color3B(200, 200, 200));
    this->setGlobalZOrder(Constants::ZOrder::FLOOR + 1);
    
    return true;
}

void Spike::setTriggered(bool triggered)
{
    _triggered = triggered;
    if (_triggered)
    {
        this->setColor(Color3B::WHITE);
    }
    else
    {
        this->setColor(Color3B(200, 200, 200));
        _damageTimer = 0.0f;
    }
}

void Spike::updateState(float dt, Player* player)
{
    if (!player || player->isDead())
    {
        if (_triggered)
        {
            setTriggered(false);
        }
        return;
    }
    
    bool isStepped = this->getBoundingBox().containsPoint(player->getPosition());
    
    if (isStepped)
    {
        if (!_triggered)
        {
            setTriggered(true);
            _damageTimer = 0.0f;  // 立即结算第一次伤害
        }
        
        _damageTimer -= dt;
        if (_damageTimer <= 0.0f)
        {
            player->takeDamage(_damagePerTick);
            _damageTimer = _damageInterval;
        }
    }
    else
    {
        if (_triggered)
        {
            setTriggered(false);
        }
    }
}

// ==================== Box ====================

Box* Box::create(BoxType type)
{
    Box* box = new (std::nothrow) Box();
    if (box && box->initWithType(type))
    {
        box->autorelease();
        return box;
    }
    CC_SAFE_DELETE(box);
    return nullptr;
}

bool Box::initWithType(BoxType type)
{
    _boxType = type;
    
    std::string texturePath;
    switch (type)
    {
        case BoxType::NORMAL:
            texturePath = "Map/Barrier/Box_normal.png";
            break;
        case BoxType::LIGHT:
            texturePath = "Map/Barrier/Box_light.png";
            break;
        case BoxType::DARK:
            texturePath = "Map/Barrier/Box_dark.png";
            break;
    }
    
    if (!Sprite::initWithFile(texturePath))
    {
        return false;
    }
    
    // 缩放到地砖大小
    float targetSize = Constants::FLOOR_TILE_SIZE;
    float scale = targetSize / this->getContentSize().width;
    this->setScale(scale);
    
    this->setGlobalZOrder(Constants::ZOrder::WALL_ABOVE);
    this->setTag(Constants::Tag::WALL);  // 设置为墙壁标签，子弹会检测
    
    return true;
}

// ==================== Pillar ====================

Pillar* Pillar::create(PillarType type)
{
    Pillar* pillar = new (std::nothrow) Pillar();
    if (pillar && pillar->initWithType(type))
    {
        pillar->autorelease();
        return pillar;
    }
    CC_SAFE_DELETE(pillar);
    return nullptr;
}

bool Pillar::initWithType(PillarType type)
{
    _pillarType = type;
    
    std::string texturePath;
    switch (type)
    {
        case PillarType::CLEAR:
            texturePath = "Map/Barrier/Pillar_clear.png";
            break;
        case PillarType::BROKEN:
            texturePath = "Map/Barrier/Pillar_broken.png";
            break;
        case PillarType::GLASSES:
            texturePath = "Map/Barrier/Pillar_glasses.png";
            break;
    }
    
    if (!Sprite::initWithFile(texturePath))
    {
        return false;
    }
    
    // 缩放到地砖大小
    float targetSize = Constants::FLOOR_TILE_SIZE;
    float scale = targetSize / this->getContentSize().width;
    this->setScale(scale);
    
    this->setGlobalZOrder(Constants::ZOrder::WALL_ABOVE);
    this->setTag(Constants::Tag::WALL);  // 设置为墙壁标签，子弹会检测
    
    return true;
}
