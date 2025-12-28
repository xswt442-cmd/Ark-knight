#include "KongKaZi.h"
#include "Entities/Player/Player.h"

static const int KONG_MOVE_ACTION_TAG = 0xB001; // 移动循环动作 tag
static const int KONG_HIT_ACTION_TAG  = 0xB002; // 命中/伤害播放动作 tag
static const int KONG_WINDUP_ACTION_TAG = 0xB003; // 攻击前摇动作 tag

KongKaZi::KongKaZi()
    : _moveAnimation(nullptr)
    , _attackAnimation(nullptr)
    , _dieAnimation(nullptr)
    , _roomBounds(cocos2d::Rect::ZERO)
    , _hasRoomBounds(false)
{
}

KongKaZi::~KongKaZi()
{
    CC_SAFE_RELEASE(_moveAnimation);
    CC_SAFE_RELEASE(_attackAnimation);
    CC_SAFE_RELEASE(_dieAnimation);
}

bool KongKaZi::init()
{
    if (!Enemy::init())
    {
        return false;
    }

    // 设置敌人类型（近战）
    setEnemyType(EnemyType::MELEE);

    setupKongKaZiAttributes();
    loadAnimations();

    // 设置初始精灵（使用移动动画第一帧）
    if (_moveAnimation)
    {
        auto frames = _moveAnimation->getFrames();
        if (!frames.empty())
        {
            auto sprite = cocos2d::Sprite::createWithSpriteFrame(frames.front()->getSpriteFrame());
            this->bindSprite(sprite);
        }
    }

    GAME_LOG("KongKaZi enemy created");
    return true;
}

void KongKaZi::update(float dt)
{
    // 如果死亡则不再更新逻辑
    if (_currentState == EntityState::DIE)
    {
        return;
    }

    Enemy::update(dt);

    // 限制位置在房间边界内（复用 Ayao 的行为）
    if (_hasRoomBounds)
    {
        cocos2d::Vec2 pos = this->getPosition();
        pos.x = std::max(_roomBounds.getMinX(), std::min(pos.x, _roomBounds.getMaxX()));
        pos.y = std::max(_roomBounds.getMinY(), std::min(pos.y, _roomBounds.getMaxY()));
        this->setPosition(pos);
    }
}

void KongKaZi::setupKongKaZiAttributes()
{
    // 设置基础属性（可根据需要调整数值）
    setMaxHP(5000);
    setHP(5000);
    setAttack(750);
    setMoveSpeed(150.0f);

    // AI 参数
    setSightRange(1000.0f);
    setAttackRange(40.0f);
    setAttackCooldown(1.5f);
    setAttackWindup(0.5f);
}

void KongKaZi::loadAnimations()
{
    using namespace cocos2d;

    // 移动动画
    Vector<SpriteFrame*> moveFrames;
    for (int i = 1; i <= 5; i++)
    {
        char filename[256];
        sprintf(filename, "Enemy/KongKaZi/KongKaZi_Move/KongKaZi_Move_%04d.png", i);
        auto sprite = Sprite::create(filename);
        if (sprite)
        {
            moveFrames.pushBack(sprite->getSpriteFrame());
        }
    }
    if (!moveFrames.empty())
    {
        _moveAnimation = Animation::createWithSpriteFrames(moveFrames, 0.1f);
        _moveAnimation->retain();
    }

    // 攻击动画
    Vector<SpriteFrame*> attackFrames;
    for (int i = 1; i <= 9; i++)
    {
        char filename[256];
        sprintf(filename, "Enemy/KongKaZi/KongKaZi_Attack/KongKaZi_Attack_%04d.png", i);
        auto sprite = Sprite::create(filename);
        if (sprite)
        {
            attackFrames.pushBack(sprite->getSpriteFrame());
        }
    }
    if (!attackFrames.empty())
    {
        _attackAnimation = Animation::createWithSpriteFrames(attackFrames, 0.1f);
        _attackAnimation->retain();
    }

    // 死亡动画
    Vector<SpriteFrame*> dieFrames;
    for (int i = 1; i <= 8; i++)
    {
        char filename[256];
        sprintf(filename, "Enemy/KongKaZi/KongKaZi_Die/KongKaZi_Die_%04d.png", i);
        auto sprite = Sprite::create(filename);
        if (sprite)
        {
            dieFrames.pushBack(sprite->getSpriteFrame());
        }
    }
    if (!dieFrames.empty())
    {
        _dieAnimation = Animation::createWithSpriteFrames(dieFrames, 0.12f);
        _dieAnimation->retain();
    }
}

void KongKaZi::attack()
{
    if (!canAttack())
    {
        return;
    }

    setState(EntityState::ATTACK);
    resetAttackCooldown();

    GAME_LOG("KongKaZi attacks!");

    // 播放攻击前摇动画（与 Ayao 一致的处理）
    if (_attackAnimation && _sprite)
    {
        auto moveAct = _sprite->getActionByTag(KONG_MOVE_ACTION_TAG);
        if (moveAct)
        {
            _sprite->stopAction(moveAct);
        }

        auto prevWind = _sprite->getActionByTag(KONG_WINDUP_ACTION_TAG);
        if (prevWind)
        {
            _sprite->stopAction(prevWind);
        }

        _attackAnimation->setRestoreOriginalFrame(true);
        auto animate = Animate::create(_attackAnimation);
        animate->setTag(KONG_WINDUP_ACTION_TAG);
        _sprite->runAction(animate);

        float windup = this->getAttackWindup();
        auto delay = DelayTime::create(windup);
        auto callback = CallFunc::create([this]() {
            if (_currentState == EntityState::ATTACK)
            {
                setState(EntityState::IDLE);
            }
        });
        auto seq = Sequence::create(delay, callback, nullptr);
        this->runAction(seq);
    }
    else
    {
        float windup = this->getAttackWindup();
        auto delay = DelayTime::create(windup);
        auto callback = CallFunc::create([this]() {
            if (_currentState == EntityState::ATTACK)
            {
                setState(EntityState::IDLE);
            }
        });
        auto seq = Sequence::create(delay, callback, nullptr);
        this->runAction(seq);
    }
}

void KongKaZi::die()
{
    // 先执行基类处理（会处理可能的红色标记爆炸与 KongKaZi 生成功能）
    Enemy::die();

    // 防止重复处理死亡逻辑
    if (_currentState == EntityState::DIE)
    {
        // 仍然继续播放本类死亡动画（不直接 return）
    }

    // 播放死亡动画（与 Ayao 保持一致）
    setState(EntityState::DIE);
    // 标记为不存活
    // 停止所有动作
    this->stopAllActions();
    if (_sprite)
    {
        _sprite->stopAllActions();
        _sprite->setVisible(true);
        _sprite->setOpacity(255);
    }

    if (_dieAnimation && _sprite)
    {
        auto animate = Animate::create(_dieAnimation);
        auto fadeOut = FadeOut::create(0.5f);
        auto removeCallback = CallFunc::create([this]() {
            this->removeFromParent();
        });
        auto sequence = Sequence::create(animate, fadeOut, removeCallback, nullptr);
        _sprite->runAction(sequence);
    }
    else
    {
        if (_sprite)
        {
            auto fadeOut = FadeOut::create(0.5f);
            auto removeCallback = CallFunc::create([this]() {
                this->removeFromParent();
            });
            auto sequence = Sequence::create(fadeOut, removeCallback, nullptr);
            _sprite->runAction(sequence);
        }
        else
        {
            this->removeFromParent();
        }
    }
}

void KongKaZi::move(const cocos2d::Vec2& direction, float dt)
{
    // 在攻击或死亡阶段禁止移动（与 Ayao 保持一致）
    if (_currentState == EntityState::ATTACK || _currentState == EntityState::DIE)
    {
        Character::move(cocos2d::Vec2::ZERO, dt);
        return;
    }

    const float STOP_THRESHOLD_SQ = 1.0f;
    if (direction.lengthSquared() <= STOP_THRESHOLD_SQ)
    {
        if (_sprite)
        {
            auto act = _sprite->getActionByTag(KONG_MOVE_ACTION_TAG);
            if (act)
            {
                _sprite->stopAction(act);
            }

            if (_moveAnimation)
            {
                auto frames = _moveAnimation->getFrames();
                if (!frames.empty())
                {
                    _sprite->setSpriteFrame(frames.front()->getSpriteFrame());
                }
            }
        }

        Character::move(cocos2d::Vec2::ZERO, dt);
        return;
    }

    // 有移动向量：先调用基类进行实际移动
    cocos2d::Vec2 dirNorm = direction.getNormalized();
    Character::move(dirNorm, dt);

    // 设置左右朝向并播放移动动画
    if (_sprite && _moveAnimation)
    {
        _sprite->setFlippedX(dirNorm.x < 0.0f);

        if (!_sprite->getActionByTag(KONG_MOVE_ACTION_TAG))
        {
            auto animate = Animate::create(_moveAnimation);
            auto repeat = RepeatForever::create(animate);
            repeat->setTag(KONG_MOVE_ACTION_TAG);
            _sprite->runAction(repeat);
        }
    }
}

void KongKaZi::playAttackAnimation()
{
    if (!_sprite) return;
    if (_currentState == EntityState::DIE) return;

    if (_currentState != EntityState::ATTACK)
    {
        setState(EntityState::ATTACK);
    }

    // 停止移动循环
    auto moveAct = _sprite->getActionByTag(KONG_MOVE_ACTION_TAG);
    if (moveAct)
    {
        _sprite->stopAction(moveAct);
    }

    // 停止前摇（如果有）
    auto wind = _sprite->getActionByTag(KONG_WINDUP_ACTION_TAG);
    if (wind)
    {
        _sprite->stopAction(wind);
    }

    if (_attackAnimation)
    {
        auto prevHit = _sprite->getActionByTag(KONG_HIT_ACTION_TAG);
        if (prevHit)
        {
            _sprite->stopAction(prevHit);
        }

        _attackAnimation->setRestoreOriginalFrame(true);
        auto animate = Animate::create(_attackAnimation);
        animate->setTag(KONG_HIT_ACTION_TAG);

        auto callback = CallFunc::create([this]() {
            if (_moveAnimation && _sprite)
            {
                auto frames = _moveAnimation->getFrames();
                if (!frames.empty())
                {
                    _sprite->setSpriteFrame(frames.front()->getSpriteFrame());
                }
            }
            if (_currentState != EntityState::DIE)
            {
                setState(EntityState::IDLE);
            }
        });
        auto seq = Sequence::create(animate, callback, nullptr);
        seq->setTag(KONG_HIT_ACTION_TAG);
        _sprite->runAction(seq);
    }
}