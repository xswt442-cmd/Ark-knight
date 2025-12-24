#include "GameEntity.h"

GameEntity::GameEntity()
    : _sprite(nullptr)
    , _hp(100)
    , _maxHP(100)
    , _isAlive(true)
    , _hitInvulTimer(0.0f) // 初始化无敌计时器
{
}

GameEntity::~GameEntity()
{
}

bool GameEntity::init()
{
    if (!Node::init())
    {
        return false;
    }

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

    // 检查死亡（保持原逻辑）
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
    // 过滤负值与已死亡
    if (!_isAlive || damage < 0)
    {
        return;
    }

    // 如果处于短时间受击无敌期，则忽略本次伤害（避免多帧重复命中）
    if (_hitInvulTimer > 0.0f)
    {
        return;
    }

    // 应用伤害
    _hp -= damage;
    if (_hp < 0)
    {
        _hp = 0;
    }

    // 设置短暂无敌窗口，防止短时间重复受击
    _hitInvulTimer = HIT_INVUL_DURATION;

    GAME_LOG("Entity takes %d damage, HP: %d/%d", damage, _hp, _maxHP);

    if (_hp <= 0)
    {
        _isAlive = false;
        die();
        return;
    }

    // 受击视觉（闪烁）
    if (_sprite != nullptr)
    {
        _sprite->stopActionByTag(100);
        _sprite->setVisible(true);

        auto blink = Blink::create(0.2f, 2);
        auto show = Show::create();
        auto sequence = Sequence::create(blink, show, nullptr);
        sequence->setTag(100);
        _sprite->runAction(sequence);
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
    this->stopAllActions();
    this->unscheduleUpdate();

    if (_sprite != nullptr)
    {
        auto fadeOut = FadeOut::create(0.5f);
        auto scaleDown = ScaleTo::create(0.5f, 0.0f);
        auto spawn = Spawn::create(fadeOut, scaleDown, nullptr);
        auto hide = Hide::create();
        auto sequence = Sequence::create(spawn, hide, nullptr);

        this->runAction(sequence);
    }
    else
    {
        this->setVisible(false);
    }
}
