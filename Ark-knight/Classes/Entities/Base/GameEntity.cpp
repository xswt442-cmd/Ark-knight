#include "GameEntity.h"

GameEntity::GameEntity()
    : _sprite(nullptr)
    , _hp(100)
    , _maxHP(100)
    , _isAlive(true)
    , _hitInvulTimer(0.0f)
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

    // 更新受击无敌计时器
    if (_hitInvulTimer > 0.0f)
    {
        _hitInvulTimer -= dt;
        if (_hitInvulTimer < 0.0f) _hitInvulTimer = 0.0f;
    }
    
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
    // 向后兼容：原来的无返回值接口仍可使用
    takeDamageReported(damage);
}

int GameEntity::takeDamageReported(int damage)
{
    // 返回“实际对实体造成的 HP 减少”
    if (!_isAlive || damage <= 0)
    {
        return 0;
    }

    // 如果处于短时间受击无敌期，则忽略本次伤害（避免多帧重复命中）
    if (_hitInvulTimer > 0.0f)
    {
        return 0;
    }
    
    int oldHP = _hp;

    _hp -= damage;
    if (_hp < 0)
    {
        _hp = 0;
    }

    // 设置短暂无敌窗口，防止短时间重复受击
    _hitInvulTimer = HIT_INVUL_DURATION;
    
    int applied = oldHP - _hp;
    GAME_LOG("Entity takes %d damage, HP: %d/%d", applied, _hp, _maxHP);
    
    // 检查是否死亡
    if (_hp <= 0)
    {
        // 立即标记为死亡，避免后续再触发受击/动作
        _isAlive = false;
        die();
        return applied;  // 死亡后不播放受击效果，但返回实际值
    }
    
    // 受击效果 - 闪烁
    if (_sprite != nullptr)
    {
        // 先停止之前的闪烁动作，避免叠加导致精灵消失
        _sprite->stopActionByTag(100);
        _sprite->setVisible(true);  // 确保可见
        
        auto blink = Blink::create(0.2f, 2);
        auto show = Show::create();  // 闪烁结束后确保显示
        auto sequence = Sequence::create(blink, show, nullptr);
        sequence->setTag(100);  // 设置标签用于停止
        _sprite->runAction(sequence);
    }

    return applied;
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
