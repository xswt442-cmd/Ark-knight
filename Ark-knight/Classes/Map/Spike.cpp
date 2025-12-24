#include "Spike.h"
#include "Entities/Player/Player.h"

USING_NS_CC;

Spike* Spike::createSpike(const std::string& texturePath)
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
    this->setGlobalZOrder(Constants::ZOrder::FLOOR + 1);  // 高于地板，低于玩家
    
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

void Spike::updateState(float dt, bool isStepped, Player* player)
{
    if (isStepped)
    {
        if (!_triggered)
        {
            setTriggered(true);
            _damageTimer = 0.0f;  // 立即结算第一次伤害
        }
        
        _damageTimer -= dt;
        if (_damageTimer <= 0.0f && player)
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
