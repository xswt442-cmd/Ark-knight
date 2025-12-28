#include "IronLightCup.h"
#include "cocos2d.h"
#include "Entities/Player/Player.h"
#include <algorithm>

USING_NS_CC;

static const int IRONLIGHT_MOVE_ACTION_TAG = 0x6E01;
static const int IRONLIGHT_DIE_ACTION_TAG  = 0x6E02;

IronLightCup::IronLightCup()
    : _moveAnimation(nullptr)
    , _dieAnimation(nullptr)
    , _roomBounds(Rect::ZERO)
    , _hasRoomBounds(false)
    , _patrolTimer(0.0f)
    , _patrolInterval(1.0f)
    , _patrolDirection(Vec2::ZERO)
{
}

IronLightCup::~IronLightCup()
{
    if (_moveAnimation) { _moveAnimation->release(); _moveAnimation = nullptr; }
    if (_dieAnimation) { _dieAnimation->release(); _dieAnimation = nullptr; }
}

bool IronLightCup::init()
{
    if (!Enemy::init()) return false;

    setEnemyType(EnemyType::MELEE);

    // 设置为“35 次击中死亡”（通过单次扣血实现）
    setMaxHP(10);
    setHP(getMaxHP());

    setMoveSpeed(40.0f);

    // 无视野/攻击相关（不会主动寻敌）
    setSightRange(0.0f);
    setAttackRange(0.0f);

    loadAnimations();

    // 绑定初始精灵：优先 move 第一帧
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
        // 兜底占位
        auto sprite = Sprite::create();
        sprite->setTextureRect(Rect(0,0,16,16));
        sprite->setColor(Color3B::GRAY);
        this->bindSprite(sprite);
    }

    // 播放移动动画（若存在）
    if (_sprite && _moveAnimation)
    {
        auto animate = Animate::create(_moveAnimation);
        auto repeat = RepeatForever::create(animate);
        repeat->setTag(IRONLIGHT_MOVE_ACTION_TAG);
        _sprite->runAction(repeat);
    }

    // 初始化巡逻节奏
    _patrolInterval = 0.8f + CCRANDOM_0_1() * 1.2f;
    _patrolTimer = 0.0f;
    _patrolDirection = Vec2::ZERO;

    GAME_LOG("IronLightCup initialized (HP=35, single-hit per contact)");

    return true;
}

void IronLightCup::loadAnimations()
{
    // 载入移动帧（假定 6 帧，找不到会记录日志）
    Vector<SpriteFrame*> moveFrames;
    for (int i = 1; i <= 7; ++i)
    {
        char filename[256];
        sprintf(filename, "Enemy/TangHuang&&Iron LightCup/IronLightCup/IronLightCup_Move/IronLightCup_Move_%04d.png", i);

        SpriteFrame* frame = nullptr;
        auto s = Sprite::create(filename);
        if (s) frame = s->getSpriteFrame();

        if (!frame)
        {
            char basename[128];
            sprintf(basename, "IronLightMove_Move_%04d.png", i);
            frame = SpriteFrameCache::getInstance()->getSpriteFrameByName(basename);
        }

        if (frame) moveFrames.pushBack(frame);
        else GAME_LOG("IronLightCup: failed to load move frame: %s", filename);
    }

    if (!moveFrames.empty())
    {
        _moveAnimation = Animation::createWithSpriteFrames(moveFrames, 0.12f);
        _moveAnimation->retain();
    }

    // 载入死亡帧（假定 5 帧）
    Vector<SpriteFrame*> dieFrames;
    for (int i = 1; i <= 5; ++i)
    {
        char filename[256];
        sprintf(filename, "Enemy/TangHuang&&Iron LightCup/IronLightCup/IronLightCup_Die/IronLightCup_Die_%04d.png", i);

        SpriteFrame* frame = nullptr;
        auto s = Sprite::create(filename);
        if (s) frame = s->getSpriteFrame();

        if (!frame)
        {
            char basename[128];
            sprintf(basename, "IronLightCup_Die_%04d.png", i);
            frame = SpriteFrameCache::getInstance()->getSpriteFrameByName(basename);
        }

        if (frame) dieFrames.pushBack(frame);
        else GAME_LOG("IronLightCup: failed to load die frame: %s", filename);
    }

    if (!dieFrames.empty())
    {
        _dieAnimation = Animation::createWithSpriteFrames(dieFrames, 0.12f);
        _dieAnimation->retain();
    }
}

void IronLightCup::update(float dt)
{
    if (_currentState == EntityState::DIE) return;

    Enemy::update(dt);

    // 限制在房间边界内（如果有设置）
    if (_hasRoomBounds)
    {
        Vec2 pos = this->getPosition();
        pos.x = std::max(_roomBounds.getMinX(), std::min(pos.x, _roomBounds.getMaxX()));
        pos.y = std::max(_roomBounds.getMinY(), std::min(pos.y, _roomBounds.getMaxY()));
        this->setPosition(pos);
    }

    // 无其它被动状态
}

void IronLightCup::executeAI(Player* /*player*/, float dt)
{
    // 不索敌、不攻击，仅随意移动（随机巡逻）
    _patrolTimer += dt;
    if (_patrolTimer >= _patrolInterval)
    {
        _patrolTimer = 0.0f;
        _patrolInterval = 0.8f + CCRANDOM_0_1() * 1.2f;

        // 随机方向或静止
        float r = CCRANDOM_0_1();
        if (r < 0.3f)
        {
            _patrolDirection = Vec2::ZERO;
        }
        else
        {
            float angle = CCRANDOM_MINUS1_1() * M_PI; // -pi..pi
            _patrolDirection = Vec2(cosf(angle), sinf(angle));
            _patrolDirection.normalize();
        }
    }

    // 调用基类移动（基类会依据 _moveSpeed 做实际位移）
    if (_patrolDirection != Vec2::ZERO)
    {
        move(_patrolDirection, dt);
    }
}

void IronLightCup::takeDamage(int damage)
{
    if (isDead()) return;

    // 每次固定计 1 点伤害。
    // 直接调用基类的 takeDamageReported，避免被附近 Cup 吸收（Enemy::takeDamageReported 的分担逻辑）。
    GameEntity::takeDamageReported(1);
}

int IronLightCup::takeDamageReported(int damage)
{
    // 强制所有来源的伤害对 IronLightCup 只造成 1 点
    if (!_isAlive || damage <= 0) return 0;

    // 保持受击无敌判断，避免短时间重复受击
    if (_hitInvulTimer > 0.0f) return 0;

    // 直接走 GameEntity 的处理，造成 1 点并触发闪烁 / 无敌 / 死亡处理
    return GameEntity::takeDamageReported(1);
}

void IronLightCup::die()
{
    // 先触发基类逻辑（红色标记 / KongKaZi 等）
    Enemy::die();

    // 停止移动动画并播放死亡动画一次，然后移除
    if (_sprite)
    {
        _sprite->stopActionByTag(IRONLIGHT_MOVE_ACTION_TAG);

        if (_dieAnimation)
        {
            auto animate = Animate::create(_dieAnimation);
            animate->setTag(IRONLIGHT_DIE_ACTION_TAG);

            // 计算总时长并在结束后移除
            float total = _dieAnimation->getDelayPerUnit() * (float)_dieAnimation->getFrames().size();
            auto seq = Sequence::create(animate, DelayTime::create(0.0f), CallFunc::create([this]() {
                this->removeFromParentAndCleanup(true);
            }), nullptr);
            _sprite->runAction(seq);
            return;
        }
    }

    // 没有死亡动画则直接移除
    this->removeFromParentAndCleanup(true);
}