#include "IronLance.h"
#include "cocos2d.h"
#include "Entities/Player/Player.h"

USING_NS_CC;

static const int IRONL_MOVE_ACTION_TAG = 0xD201;

IronLance::IronLance()
    : _moveAnimation(nullptr)
    , _dieAnimation(nullptr)
    , _roomBounds(Rect::ZERO)
    , _hasRoomBounds(false)
{
}

IronLance::~IronLance()
{
    CC_SAFE_RELEASE(_moveAnimation);
    CC_SAFE_RELEASE(_dieAnimation);
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

    // 尝试分别加载 Move 和 Die 动画帧（优先加载 Move，但也同时尝试加载 Die）
    Vector<SpriteFrame*> moveFrames;
    for (int i = 1; i <= 8; ++i)
    {
        char filename[256];
        sprintf(filename, "Enemy/XinXing&&Iron Lance/Iron Lance/Iron Lance_Move/IronLance_Move_%04d.png", i);

        SpriteFrame* frame = nullptr;
        auto s = Sprite::create(filename);
        if (s)
        {
            frame = s->getSpriteFrame(); // 有些平台从文件创建的 Sprite 能返回 SpriteFrame
        }

        // 回退：尝试从 SpriteFrameCache 读取（若资源打包进 plist）
        if (!frame)
        {
            char basename[128];
            sprintf(basename, "IronLance_Move_%04d.png", i);
            frame = SpriteFrameCache::getInstance()->getSpriteFrameByName(basename);
        }

        if (frame)
        {
            moveFrames.pushBack(frame);
        }
        else
        {
            // 记录加载失败的帧以便调试（不会改变功能）
            GAME_LOG("IronLance: failed to load move frame: %s", filename);
        }
    }

    Vector<SpriteFrame*> dieFrames;
    for (int i = 1; i <= 5; ++i)
    {
        char filename[256];
        sprintf(filename, "Enemy/XinXing&&Iron Lance/Iron Lance/Iron Lance_Die/IronLance_Die_%04d.png", i);

        SpriteFrame* frame = nullptr;
        auto s = Sprite::create(filename);
        if (s)
        {
            frame = s->getSpriteFrame();
        }

        if (!frame)
        {
            char basename[128];
            sprintf(basename, "IronLance_Die_%04d.png", i);
            frame = SpriteFrameCache::getInstance()->getSpriteFrameByName(basename);
        }

        if (frame)
        {
            dieFrames.pushBack(frame);
        }
        else
        {
            GAME_LOG("IronLance: failed to load die frame: %s", filename);
        }
    }

    if (!moveFrames.empty())
    {
        _moveAnimation = Animation::createWithSpriteFrames(moveFrames, 0.12f);
        _moveAnimation->retain();
    }

    if (!dieFrames.empty())
    {
        _dieAnimation = Animation::createWithSpriteFrames(dieFrames, 0.12f);
        _dieAnimation->retain();
    }

    // 初始精灵：优先使用 Move 的第一帧；若无 Move 则使用 Die 的第一帧；否则兜底简单精灵
    if (_moveAnimation)
    {
        auto frames = _moveAnimation->getFrames();
        if (!frames.empty())
        {
            auto sprite = Sprite::createWithSpriteFrame(frames.front()->getSpriteFrame());
            this->bindSprite(sprite);
        }
    }
    else if (_dieAnimation)
    {
        auto frames = _dieAnimation->getFrames();
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

void IronLance::executeAI(Player* , float dt)
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

    // 移动动画由 move() 管理（遵循 Ayao 的逻辑），因此不在此处强制启动动画。
}

void IronLance::move(const Vec2& direction, float dt)
{
    // IronLance 不会攻击，但仍需在死亡时停止移动
    if (_currentState == EntityState::DIE)
    {
        Character::move(Vec2::ZERO, dt);
        return;
    }

    const float STOP_THRESHOLD_SQ = 1.0f;
    if (direction.lengthSquared() <= STOP_THRESHOLD_SQ)
    {
        // 停止移动动作（如果存在）
        if (_sprite)
        {
            auto act = _sprite->getActionByTag(IRONL_MOVE_ACTION_TAG);
            if (act)
            {
                _sprite->stopAction(act);
            }

            // 恢复到移动动画的第一帧（通常为站立/闭嘴状态）
            if (_moveAnimation)
            {
                auto frames = _moveAnimation->getFrames();
                if (!frames.empty())
                {
                    auto firstFrame = frames.front()->getSpriteFrame();
                    if (firstFrame)
                    {
                        _sprite->setSpriteFrame(firstFrame);
                    }
                }
            }
        }

        // 调用基类移动以保持速度/状态一致
        Character::move(Vec2::ZERO, dt);
        return;
    }

    // 有移动向量：先调用基类以实际移动实体
    Vec2 dirNorm = direction.getNormalized();
    Character::move(dirNorm, dt);

    // 设置朝向（左右），并确保移动动画在循环播放
    if (_sprite && _moveAnimation)
    {
        _sprite->setFlippedX(dirNorm.x < 0.0f);

        // 如果移动动画未在播放，则启动循环播放并打上 tag
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

    // 如果存在可用的死亡动画帧（优先使用 _dieAnimation），把它播放为一次性视觉，然后移除节点
    Animation* deathAnim = _dieAnimation ? _dieAnimation : _moveAnimation;

    if (_sprite && deathAnim)
    {
        // 停止循环移动动画
        auto moveAct = _sprite->getActionByTag(IRONL_MOVE_ACTION_TAG);
        if (moveAct) _sprite->stopAction(moveAct);

        // 播放一次死亡动画（若只有 Move 帧也能适用）
        auto animate = Animate::create(deathAnim);
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