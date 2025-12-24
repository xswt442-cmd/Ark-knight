#include "IronLance.h"
#include "cocos2d.h"
#include "Entities/Player/Player.h"

USING_NS_CC;

static const int IRONL_MOVE_ACTION_TAG = 0xD201;

IronLance::IronLance()
    : _moveAnimation(nullptr)
    , _roomBounds(Rect::ZERO)
    , _hasRoomBounds(false)
{
}

IronLance::~IronLance()
{
    CC_SAFE_RELEASE(_moveAnimation);
}

bool IronLance::init()
{
    if (!Enemy::init()) return false;

    setEnemyType(EnemyType::MELEE);

    // 基础属性
    setMaxHP(45);
    setHP(45);

    setAttack(0);
    setMoveSpeed(60.0f);

    // 不索敌、不攻击
    setSightRange(0.0f);
    setAttackRange(0.0f);
    setAttackCooldown(10.0f);

    // 尝试加载 Move 动画帧（优先）
    Vector<SpriteFrame*> moveFrames;
    for (int i = 1; i <= 8; ++i)
    {
        char filename[256];
        sprintf(filename, "Enemy/XinXing&&Iron Lance/Iron Lance/Iron Lance_Move/IronLance_Move_%04d.png", i);
        auto s = Sprite::create(filename);
        if (s && s->getSpriteFrame()) moveFrames.pushBack(s->getSpriteFrame());
    }

    // 若没有 Move，尝试加载 Die（部分资源只有 Die 命名）
    if (moveFrames.empty())
    {
        for (int i = 1; i <= 5; ++i)
        {
            char filename[256];
            sprintf(filename, "Enemy/XinXing&&Iron Lance/Iron Lance/Iron Lance_Die/IronLance_Die_%04d.png", i);
            auto s = Sprite::create(filename);
            if (s && s->getSpriteFrame()) moveFrames.pushBack(s->getSpriteFrame());
        }
    }

    if (!moveFrames.empty())
    {
        _moveAnimation = Animation::createWithSpriteFrames(moveFrames, 0.12f);
        _moveAnimation->retain();
    }

    // 初始精灵并缩小 0.5（使用已加载的帧的第一帧）
    if (_moveAnimation)
    {
        auto frames = _moveAnimation->getFrames();
        if (!frames.empty())
        {
            auto sprite = Sprite::createWithSpriteFrame(frames.front()->getSpriteFrame());
            this->bindSprite(sprite);
        }
    }
    else
    {
        // 兜底：简易小点精灵
        auto sprite = Sprite::create();
        sprite->setTextureRect(Rect(0,0,16,16));
        sprite->setColor(Color3B::GRAY);
        this->bindSprite(sprite);
    }

    // 初始巡逻目标等交给 Enemy::init 的默认巡逻逻辑
    GAME_LOG("IronLance created (HP=45, single-damage per hit)");
    return true;
}

void IronLance::update(float dt)
{
    if (_currentState == EntityState::DIE) return;

    Enemy::update(dt);

    // 房间边界限制
    if (_hasRoomBounds)
    {
        Vec2 pos = this->getPosition();
        pos.x = std::max(_roomBounds.getMinX(), std::min(pos.x, _roomBounds.getMaxX()));
        pos.y = std::max(_roomBounds.getMinY(), std::min(pos.y, _roomBounds.getMaxY()));
        this->setPosition(pos);
    }
}

void IronLance::executeAI(Player* /*player*/, float dt)
{
    // 只巡逻/漂移，不索敌、不攻击
    patrol(dt);

    Vec2 dir = _patrolTarget - this->getPosition();
    if (dir.lengthSquared() > 1.0f)
    {
        move(dir, dt);
    }
    else
    {
        move(Vec2::ZERO, dt);
    }

    // 保持移动动画（如果存在）
    if (_sprite && _moveAnimation)
    {
        if (!_sprite->getActionByTag(IRONL_MOVE_ACTION_TAG))
        {
            auto animate = Animate::create(_moveAnimation);
            auto repeat = RepeatForever::create(animate);
            repeat->setTag(IRONL_MOVE_ACTION_TAG);
            _sprite->runAction(repeat);
        }
    }
}

void IronLance::takeDamage(int damage)
{
    if (!_isAlive || damage <= 0) return;

    // 不论传入 damage 为多少，每次命中固定扣 1
    // 直接调用基类（GameEntity / Character）的伤害方法以维持浮动显示等逻辑
    GameEntity::takeDamage(1);
}

void IronLance::die()
{
    // 先调用 Enemy::die() 以处理红色标记 / KongKaZi 生成功能（不改变实体状态）
    Enemy::die();

    // 设置为死亡状态并停止动作
    setState(EntityState::DIE);
    this->stopAllActions();
    if (_sprite)
    {
        _sprite->stopAllActions();
        _sprite->setVisible(true);
        _sprite->setOpacity(255);
    }

    // 如果存在可用的动画帧（_moveAnimation），把它当作一次性死亡视觉播放，然后移除节点
    if (_sprite && _moveAnimation)
    {
        // 停止循环移动动画
        auto moveAct = _sprite->getActionByTag(IRONL_MOVE_ACTION_TAG);
        if (moveAct) _sprite->stopAction(moveAct);

        // 播放一次动画（如果资源实际上是 Die 帧也适用）
        auto animate = Animate::create(_moveAnimation);
        // 结尾淡出并删除节点
        auto fadeOut = FadeOut::create(0.5f);
        auto removeCallback = CallFunc::create([this]() {
            if (this->getParent()) this->removeFromParent();
        });
        auto seq = Sequence::create(animate, fadeOut, removeCallback, nullptr);
        _sprite->runAction(seq);
    }
    else
    {
        // 使用基类的死亡效果并在结束后移除节点
        Character::die(); // 会调用 showDeathEffect()
        auto removeCallback = CallFunc::create([this]() {
            if (this->getParent()) this->removeFromParent();
        });
        // showDeathEffect 的动画大约 0.5s，延迟稍长以确保视觉完成
        this->runAction(Sequence::create(DelayTime::create(0.55f), removeCallback, nullptr));
    }
}