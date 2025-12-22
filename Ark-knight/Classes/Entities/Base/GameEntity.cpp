#include "GameEntity.h"

GameEntity::GameEntity()
    : _sprite(nullptr)
    , _hp(100)
    , _maxHP(100)
    , _isAlive(true)
{
}

GameEntity::~GameEntity()
{
    // Sprite会被Node自动管理，不需要手动释放
}

bool GameEntity::init()
{
    if (!Node::init())
    {
        return false;
    }
    
    // 开启update
    scheduleUpdate();
    
    return true;
}

void GameEntity::update(float dt)
{
    Node::update(dt);
    
    // 检查死亡
    if (_hp <= 0 && _isAlive)
    {
        _isAlive = false;
        die();
    }
}

void GameEntity::bindSprite(Sprite* sprite, int zOrder)
{
    if (_sprite != nullptr)
    {
        _sprite->removeFromParent();
    }
    
    _sprite = sprite;
    if (_sprite != nullptr)
    {
        this->addChild(_sprite, zOrder);
        _sprite->setGlobalZOrder(static_cast<float>(zOrder));
    }
}

void GameEntity::takeDamage(int damage)
{
    if (!_isAlive || damage < 0)
    {
        return;
    }
    
    _hp -= damage;
    if (_hp < 0)
    {
        _hp = 0;
    }
    
    GAME_LOG("Entity takes %d damage, HP: %d/%d", damage, _hp, _maxHP);
    
    // 检查是否死亡
    if (_hp <= 0)
    {
        // 立即标记为死亡，避免后续再触发受击/动作
        _isAlive = false;
        die();
        return;  // 死亡后不播放受击效果
    }
    
    // 受击效果 - 闪烁
    if (_sprite != nullptr)
    {
        auto blink = Blink::create(0.2f, 2);
        _sprite->runAction(blink);
    }
}

void GameEntity::heal(int healAmount)
{
    if (!_isAlive || healAmount < 0)
    {
        return;
    }
    
    _hp += healAmount;
    if (_hp > _maxHP)
    {
        _hp = _maxHP;
    }
    
    GAME_LOG("Entity heals %d HP, HP: %d/%d", healAmount, _hp, _maxHP);
}

Rect GameEntity::getBoundingBox() const
{
    if (_sprite != nullptr)
    {
        return _sprite->getBoundingBox();
    }
    return Rect(getPositionX(), getPositionY(), 0, 0);
}

bool GameEntity::checkCollision(GameEntity* other) const
{
    if (other == nullptr)
    {
        return false;
    }
    
    return this->getBoundingBox().intersectsRect(other->getBoundingBox());
}

void GameEntity::showDeathEffect()
{
    // 停止所有动作和更新
    this->stopAllActions();
    this->unscheduleUpdate();
    
    if (_sprite != nullptr)
    {
        // 死亡效果：淡出 + 缩小（不删除对象，只隐藏）
        auto fadeOut = FadeOut::create(0.5f);
        auto scaleDown = ScaleTo::create(0.5f, 0.0f);
        auto spawn = Spawn::create(fadeOut, scaleDown, nullptr);
        auto hide = Hide::create();
        auto sequence = Sequence::create(spawn, hide, nullptr);
        
        this->runAction(sequence);
    }
    else
    {
        // 没有精灵直接隐藏
        this->setVisible(false);
    }
}
