#include "Boat.h"
#include "Entities/Player/Player.h"
#include "Scenes/GameScene.h"
#include "UI/FloatingText.h"

USING_NS_CC;

Boat::Boat()
    : _isMoving(false)
    , _idleTimer(0.0f)
    , _lifeTimer(0.0f)
    , _collisionCount(0)
    , _absorbedCount(0)
    , _collisionCooldown(0.0f)
    , _moveChangeTimer(0.0f)
    , _roomBounds(Rect::ZERO)
    , _currentMoveDir(Vec2::ZERO)
    , _animIdle(nullptr)
    , _animMove(nullptr)
    , _animDie(nullptr)
    , _deathCallback(nullptr)
{
}

Boat::~Boat()
{
    if (_animIdle) _animIdle->release();
    if (_animMove) _animMove->release();
    if (_animDie) _animDie->release();
}

bool Boat::init()
{
    if (!Enemy::init()) return false;

    setEnemyType(EnemyType::MELEE);
    setMaxHP(50000);
    setHP(getMaxHP());
    setMoveSpeed(150.0f);
    setAttack(0);

    // 图层设置：在障碍物上方
    this->setLocalZOrder(100); 
    // 扩大1.5倍
    this->setScale(1.5f);

    loadAnimations();

    if (_animIdle)
    {
        auto animate = Animate::create(_animIdle);
        auto repeat = RepeatForever::create(animate);
        repeat->setTag(BOAT_ACTION_TAG);
        if (_sprite) _sprite->runAction(repeat);
    }

    return true;
}

void Boat::setRoomBounds(const Rect& bounds)
{
    _roomBounds = bounds;
}

void Boat::loadAnimations()
{
    auto loadAnim = [](const std::string& pathFormat, int count, float delay) -> Animation* {
        Vector<SpriteFrame*> frames;
        for (int i = 1; i <= count; ++i)
        {
            char buf[256];
            sprintf(buf, pathFormat.c_str(), i);
            auto sprite = Sprite::create(buf);
            if (sprite) frames.pushBack(sprite->getSpriteFrame());
        }
        if (frames.empty()) return nullptr;
        auto anim = Animation::createWithSpriteFrames(frames, delay);
        anim->retain();
        return anim;
    };

    _animIdle = loadAnim("Enemy/Boat/Boat_Idle/Boat_Idle_%04d.png", 4, 0.15f);
    _animMove = loadAnim("Enemy/Boat/Boat_Move/Boat_Move_%04d.png", 4, 0.1f);
    _animDie  = loadAnim("Enemy/Boat/Boat_Die/Boat_Die_%04d.png", 6, 0.1f);

    if (!_sprite && _animIdle)
    {
        auto frames = _animIdle->getFrames();
        if (!frames.empty())
        {
            auto s = Sprite::createWithSpriteFrame(frames.front()->getSpriteFrame());
            this->bindSprite(s);
        }
    }
    else if (!_sprite)
    {
        auto s = Sprite::create();
        s->setTextureRect(Rect(0,0,64,64));
        s->setColor(Color3B::MAGENTA);
        this->bindSprite(s);
    }
}

void Boat::update(float dt)
{
    if (_currentState == EntityState::DIE) return;

    Enemy::update(dt);

    if (_collisionCooldown > 0.0f) {
        _collisionCooldown -= dt;
    }

    // 1. 出场 Idle 5秒
    if (!_isMoving)
    {
        _idleTimer += dt;
        if (_idleTimer >= 5.0f)
        {
            _isMoving = true;
            pickNewDirection();
            
            if (_sprite && _animMove)
            {
                _sprite->stopActionByTag(BOAT_ACTION_TAG);
                auto animate = Animate::create(_animMove);
                auto repeat = RepeatForever::create(animate);
                repeat->setTag(BOAT_ACTION_TAG);
                _sprite->runAction(repeat);
            }
        }
        return;
    }

    // 2. 移动逻辑
    if (_isMoving)
    {
        // AI: 定时改变方向 (随机游荡)
        _moveChangeTimer -= dt;
        if (_moveChangeTimer <= 0.0f) {
            pickNewDirection();
        }

        Vec2 nextPos = getPosition() + _currentMoveDir * getMoveSpeed() * dt;
        
        // 边界检查 (Clamp)
        if (!_roomBounds.equals(Rect::ZERO))
        {
            if (nextPos.x < _roomBounds.getMinX() || nextPos.x > _roomBounds.getMaxX() ||
                nextPos.y < _roomBounds.getMinY() || nextPos.y > _roomBounds.getMaxY())
            {
                // 撞到边界：限制位置并立即换向
                nextPos.x = std::max(_roomBounds.getMinX(), std::min(nextPos.x, _roomBounds.getMaxX()));
                nextPos.y = std::max(_roomBounds.getMinY(), std::min(nextPos.y, _roomBounds.getMaxY()));
                
                pickNewDirection();
            }
        }

        setPosition(nextPos);
        updateFacing();
    }

    // 3. 碰撞检测
    checkPlayerCollision();
}

void Boat::move(const Vec2& direction, float dt)
{
    // 留空，由 update 控制
}

void Boat::pickNewDirection()
{
    int r = cocos2d::random(0, 3);
    switch (r)
    {
        case 0: _currentMoveDir = Vec2(0, 1); break; // Up
        case 1: _currentMoveDir = Vec2(0, -1); break; // Down
        case 2: _currentMoveDir = Vec2(-1, 0); break; // Left
        case 3: _currentMoveDir = Vec2(1, 0); break; // Right
    }
    
    _moveChangeTimer = cocos2d::random(1.0f, 3.0f);
    updateFacing();
}

void Boat::updateFacing()
{
    if (!_sprite) return;
    
    // 仅根据水平分量决定翻转
    if (_currentMoveDir.x > 0) {
        _sprite->setFlippedX(false);
    } else if (_currentMoveDir.x < 0) {
        _sprite->setFlippedX(true);
    }
    
    _sprite->setRotation(0);
}

void Boat::checkPlayerCollision()
{
    if (_collisionCooldown > 0.0f) return;

    auto scene = Director::getInstance()->getRunningScene();
    auto gameScene = dynamic_cast<GameScene*>(scene);
    if (!gameScene) return;
    
    auto player = gameScene->getPlayer();
    if (!player || player->isDead()) return;

    float distSq = getPosition().getDistanceSq(player->getPosition());
    float radius = 40.0f * 1.5f; 
    if (distSq < radius * radius)
    {
        _collisionCooldown = 2.0f;

        _collisionCount++;
        _absorbedCount++;

        int oldMax = player->getMaxHP();
        int newMax = std::max(1, oldMax - 500);
        player->setMaxHP(newMax);
        
        if (player->getHP() > newMax)
        {
            player->setHP(newMax);
        }

        FloatingText::show(player->getParent(), player->getPosition(), "-500 MaxHP", Color3B::MAGENTA);

        if (_collisionCount == 1)
        {
            setMoveSpeed(getMoveSpeed() * 0.5f);
        }

        // 碰撞后反向移动一段距离
        _currentMoveDir = -_currentMoveDir;
        setPosition(getPosition() + _currentMoveDir * 60.0f); 
        updateFacing();
    }
}

void Boat::takeDamage(int damage)
{
    if (_currentState == EntityState::DIE) return;

    int oldHP = _hp;
    _hp -= damage;
    if (_hp < 0) _hp = 0;

    if (_hp <= 0 && oldHP > 0)
    {
        die();
    }
}

// 【新增】实现死亡逻辑
void Boat::die()
{
    if (_currentState == EntityState::DIE) return;
    
    setState(EntityState::DIE);
    _isMoving = false;
    stopAllActions();
    
    // 播放死亡动画
    if (_sprite && _animDie)
    {
        auto animate = Animate::create(_animDie);
        auto callback = CallFunc::create([this](){
            // 动画结束后触发回调（通知 Boss）
            if (_deathCallback) _deathCallback();
            this->removeFromParentAndCleanup(true);
        });
        _sprite->runAction(Sequence::create(animate, callback, nullptr));
    }
    else
    {
        // 无动画直接触发
        if (_deathCallback) _deathCallback();
        this->removeFromParentAndCleanup(true);
    }
}

void Boat::forceDissipate()
{
    if (_currentState == EntityState::DIE) return;
    
    // 强制消失时不触发死亡回调（避免 Boss 重复调用）
    _deathCallback = nullptr;
    
    setState(EntityState::DIE);
    _isMoving = false;
    stopAllActions();

    if (_sprite && _animDie)
    {
        auto animate = Animate::create(_animDie);
        auto callback = CallFunc::create([this](){
            this->removeFromParentAndCleanup(true);
        });
        _sprite->runAction(Sequence::create(animate, callback, nullptr));
    }
    else
    {
        this->removeFromParentAndCleanup(true);
    }
}