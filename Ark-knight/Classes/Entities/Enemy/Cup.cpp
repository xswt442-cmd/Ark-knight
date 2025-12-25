#include "Cup.h"
#include "UI/FloatingText.h"
#include "Entities/Player/Player.h"
#include "cocos2d.h"
#include <algorithm>
#include <string>

USING_NS_CC;

static const int CUP_IDLE_ACTION_TAG = 0xC001;
static const int CUP_DIE_ACTION_TAG  = 0xC002;

std::vector<Cup*> Cup::_instances;

Cup::Cup()
    : _idleAnimation(nullptr)
    , _dieAnimation(nullptr)
    , _sprite(nullptr)
    , _rangeIndicator(nullptr)
    , _shareRadius(300.0f)
    , _shareRatio(0.95f)
    , _patrolTimer(0.0f)
    , _patrolInterval(1.5f)
    , _patrolDirection(Vec2::ZERO)
    , _hasRoomBounds(false)
{
}

Cup::~Cup()
{
    auto it = std::find(_instances.begin(), _instances.end(), this);
    if (it != _instances.end()) _instances.erase(it);

    if (_idleAnimation) {
        _idleAnimation->release();
        _idleAnimation = nullptr;
    }
    if (_dieAnimation) {
        _dieAnimation->release();
        _dieAnimation = nullptr;
    }
}

bool Cup::init()
{
    if (!Enemy::init())
    {
        return false;
    }

    setEnemyType(EnemyType::MELEE);
    setMaxHP(10000);
    setHP(getMaxHP());
    setMoveSpeed(80.0f);


    Sprite* initial = Sprite::create("Enemy/Cup/Cup_Idle/Cup_Idle_0001.png");
    if (!initial) {
        initial = Sprite::create();
    }
    initial->setScale(1.2f);

    // 使用默认 ZOrder (Constants::ZOrder::ENTITY)
    bindSprite(initial);

    _sprite = getSprite();

    loadAnimations();

    if (_idleAnimation && _sprite) {
        auto animate = Animate::create(_idleAnimation);
        auto repeat = RepeatForever::create(animate);
        repeat->setTag(CUP_IDLE_ACTION_TAG);
        _sprite->runAction(repeat);
    }

    _rangeIndicator = DrawNode::create();
    if (_rangeIndicator) {
        int rangeZ = Constants::ZOrder::ENTITY - 1;
        this->addChild(_rangeIndicator, rangeZ);
        _rangeIndicator->setGlobalZOrder(static_cast<float>(rangeZ));
    }

    _patrolInterval = 1.5f + CCRANDOM_0_1() * 1.5f;
    _patrolTimer = 0.0f;
    _patrolDirection = Vec2::ZERO;

    scheduleUpdate();

    if (std::find(_instances.begin(), _instances.end(), this) == _instances.end()) {
        _instances.push_back(this);
    }

    return true;
}

void Cup::loadAnimations()
{
    Vector<SpriteFrame*> idleFrames;
    for (int i = 1; i <= 12; ++i) {
        char filename[256];
        snprintf(filename, sizeof(filename), "Enemy/Cup/Cup_Idle/Cup_Idle_%04d.png", i);
        auto temp = Sprite::create(filename);
        if (temp) {
            auto frame = temp->getSpriteFrame();
            if (frame) idleFrames.pushBack(frame);
        }
    }
    if (!idleFrames.empty()) {
        _idleAnimation = Animation::createWithSpriteFrames(idleFrames, 0.14f);
        _idleAnimation->retain();
        if (_sprite) {
            SpriteFrame* first = idleFrames.front();
            if (first) _sprite->setSpriteFrame(first);
        }
    }

    Vector<SpriteFrame*> dieFrames;
    for (int i = 1; i <= 6; ++i) {
        char filename[256];
        snprintf(filename, sizeof(filename), "Enemy/Cup/Cup_Die/Cup_Die_%04d.png", i);
        auto temp = Sprite::create(filename);
        if (temp) {
            auto frame = temp->getSpriteFrame();
            if (frame) dieFrames.pushBack(frame);
        }
    }
    if (!dieFrames.empty()) {
        _dieAnimation = Animation::createWithSpriteFrames(dieFrames, 0.12f);
        _dieAnimation->retain();
    }
}

void Cup::update(float dt)
{
    if (_currentState == EntityState::DIE)
    {
        return;
    }

    Enemy::update(dt);

    // 保留原有巡逻逻辑（若不想巡逻可注释）
    _patrolTimer += dt;
    if (_patrolTimer >= _patrolInterval) {
        _patrolTimer = 0.0f;
        _patrolInterval = 1.0f + CCRANDOM_0_1() * 2.0f;
        float r = CCRANDOM_0_1();
        if (r < 0.4f) {
            _patrolDirection = Vec2::ZERO;
        } else {
            float angle = CCRANDOM_MINUS1_1() * M_PI;
            _patrolDirection = Vec2(cosf(angle), sinf(angle));
            _patrolDirection.normalize();
        }
    }

    if (_patrolDirection != Vec2::ZERO) {
        if (_sprite) {
            _sprite->setFlippedX(_patrolDirection.x < 0.0f);
        }
        move(_patrolDirection, dt);
    } else {
        if (_sprite && !_sprite->getActionByTag(CUP_IDLE_ACTION_TAG) && _idleAnimation) {
            auto animate = Animate::create(_idleAnimation);
            auto repeat = RepeatForever::create(animate);
            repeat->setTag(CUP_IDLE_ACTION_TAG);
            _sprite->runAction(repeat);
        }
        move(Vec2::ZERO, dt);
    }

    if (_rangeIndicator) {
        _rangeIndicator->clear();
        Color4F fillColor(1.0f, 0.92f, 0.0f, 0.12f);
        Color4F borderColor(1.0f, 0.92f, 0.0f, 0.35f);
        _rangeIndicator->drawSolidCircle(Vec2::ZERO, _shareRadius, 0, 64, fillColor);
        _rangeIndicator->drawCircle(Vec2::ZERO, _shareRadius, 0, 64, false, borderColor);
    }

    if (_hasRoomBounds) {
        Vec2 pos = getPosition();
        float halfW = 1.0f;
        float minX = _roomBounds.getMinX() + halfW;
        float maxX = _roomBounds.getMaxX() - halfW;
        float minY = _roomBounds.getMinY() + halfW;
        float maxY = _roomBounds.getMaxY() - halfW;
        pos.x = clampf(pos.x, minX, maxX);
        pos.y = clampf(pos.y, minY, maxY);
        setPosition(pos);
    }
}

void Cup::absorbDamage(int damage)
{
    if (damage <= 0) return;

    int absorbed = static_cast<int>(std::round(damage * _shareRatio));
    if (absorbed <= 0) return;

    int newHP = getHP() - absorbed;
    setHP(newHP);

    if (getHP() <= 0)
    {
        die();
    }
}

// 覆写 executeAI：不索敌，不靠近玩家
void Cup::executeAI(Player* player, float dt)
{
    // 故意空实现：Cup 不会基于玩家进行追击或攻击
    // 保持由 update() 控制的巡逻/静止行为
    (void)player;
    (void)dt;
}

void Cup::die()
{
    if (_currentState == EntityState::DIE)
        return;

    Enemy::die();

    setState(EntityState::DIE);
    _isAlive = false;

    unscheduleUpdate();

    auto it = std::find(_instances.begin(), _instances.end(), this);
    if (it != _instances.end()) _instances.erase(it);

    if (_dieAnimation && _sprite) {
        auto idleAct = _sprite->getActionByTag(CUP_IDLE_ACTION_TAG);
        if (idleAct) _sprite->stopAction(idleAct);

        auto animate = Animate::create(_dieAnimation);
        animate->setTag(CUP_DIE_ACTION_TAG);
        auto fade = FadeOut::create(0.4f);
        auto remove = CallFunc::create([this]() {
            this->removeFromParentAndCleanup(true);
        });
        auto seq = Sequence::create(animate, fade, remove, nullptr);
        _sprite->runAction(seq);
    } else {
        removeFromParentAndCleanup(true);
    }
}

void Cup::setRoomBounds(const Rect& bounds)
{
    _roomBounds = bounds;
    _hasRoomBounds = true;
}

const std::vector<Cup*>& Cup::getInstances()
{
    return _instances;
}