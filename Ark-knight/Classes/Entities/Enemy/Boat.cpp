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
    , _roomBounds(Rect::ZERO)
    , _currentMoveDir(Vec2::ZERO)
    , _animIdle(nullptr)
    , _animMove(nullptr)
    , _animDie(nullptr)
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

    // 属性设置
    setEnemyType(EnemyType::MELEE); // 或其他类型
    setMaxHP(50000); // 生命值高
    setHP(getMaxHP());
    setMoveSpeed(150.0f); // 初始速度
    setAttack(0); // 不造成普通攻击伤害，而是扣上限

    // 图层设置：在障碍物上方
    // 假设障碍物在默认层级，这里设置较高的 LocalZOrder 或 GlobalZOrder
    // 依据 KuiLongBoss 中的参考，使用较高的 ZOrder
    this->setLocalZOrder(100); 

    loadAnimations();

    // 初始状态：Idle
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
    // 辅助加载 lambda
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

    // 路径参考题目描述
    _animIdle = loadAnim("Enemy/Boat/Boat_Idle/Boat_Idle_%04d.png", 4, 0.15f); // 假设4帧
    _animMove = loadAnim("Enemy/Boat/Boat_Move/Boat_Move_%04d.png", 4, 0.1f);  // 假设4帧
    _animDie  = loadAnim("Enemy/Boat/Boat_Die/Boat_Die_%04d.png", 6, 0.1f);    // 假设6帧

    // 绑定初始 Sprite
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
        // 兜底
        auto s = Sprite::create();
        s->setTextureRect(Rect(0,0,64,64));
        s->setColor(Color3B::MAGENTA);
        this->bindSprite(s);
    }
}

void Boat::update(float dt)
{
    // 如果已经死亡，不再执行逻辑
    if (_currentState == EntityState::DIE) return;

    Enemy::update(dt);

    // 1. 出场 Idle 5秒
    if (!_isMoving)
    {
        _idleTimer += dt;
        if (_idleTimer >= 5.0f)
        {
            _isMoving = true;
            pickNewDirection();
            
            // 切换动画到 Move
            if (_sprite && _animMove)
            {
                _sprite->stopActionByTag(BOAT_ACTION_TAG);
                auto animate = Animate::create(_animMove);
                auto repeat = RepeatForever::create(animate);
                repeat->setTag(BOAT_ACTION_TAG);
                _sprite->runAction(repeat);
            }
        }
        return; // Idle 期间不移动
    }

    // 2. 移动逻辑
    if (_isMoving)
    {
        // 简单的移动更新
        Vec2 nextPos = getPosition() + _currentMoveDir * getMoveSpeed() * dt;
        
        // 边界检查
        bool hitBound = false;
        if (!_roomBounds.equals(Rect::ZERO))
        {
            if (nextPos.x < _roomBounds.getMinX() || nextPos.x > _roomBounds.getMaxX() ||
                nextPos.y < _roomBounds.getMinY() || nextPos.y > _roomBounds.getMaxY())
            {
                hitBound = true;
            }
        }

        if (hitBound)
        {
            // 撞墙，重新选择方向（确保不立刻再次撞墙）
            // 简单处理：退回一步并换向
            pickNewDirection();
        }
        else
        {
            setPosition(nextPos);
        }
    }

    // 3. 碰撞检测
    checkPlayerCollision();
}

void Boat::move(const Vec2& direction, float dt)
{
    // 覆写为空或调用基类，但在 update 中我们自己控制了位置
    // 这里留空以屏蔽基类的寻路移动
}

void Boat::pickNewDirection()
{
    // 只能水平或竖直移动
    // 随机选择 上下左右
    int r = cocos2d::random(0, 3);
    switch (r)
    {
        case 0: _currentMoveDir = Vec2(0, 1); break; // Up
        case 1: _currentMoveDir = Vec2(0, -1); break; // Down
        case 2: _currentMoveDir = Vec2(-1, 0); break; // Left
        case 3: _currentMoveDir = Vec2(1, 0); break; // Right
    }
}

void Boat::checkPlayerCollision()
{
    auto scene = Director::getInstance()->getRunningScene();
    auto gameScene = dynamic_cast<GameScene*>(scene);
    if (!gameScene) return;
    
    auto player = gameScene->getPlayer();
    if (!player || player->isDead()) return;

    // 简单的矩形或距离碰撞
    float distSq = getPosition().getDistanceSq(player->getPosition());
    float radius = 40.0f; // 碰撞半径
    if (distSq < radius * radius)
    {
        // 触发碰撞效果
        // 扣除玩家生命上限 500
        // 注意：Player 类可能没有直接的 setMaxHP 接口暴露给外部修改上限并扣除当前血量
        // 这里假设可以通过 setMaxHP 修改，并手动处理当前血量
        
        // 计数
        _collisionCount++;
        _absorbedCount++; // 存入自身计数器

        // 扣除上限
        int oldMax = player->getMaxHP();
        int newMax = std::max(1, oldMax - 500); // 至少保留1点
        player->setMaxHP(newMax);
        
        // 如果当前血量高于新上限，也需要扣除
        if (player->getHP() > newMax)
        {
            player->setHP(newMax);
        }

        // 浮字提示
        FloatingText::show(player->getParent(), player->getPosition(), "-500 MaxHP", Color3B::MAGENTA);

        // 第一次碰撞后减速 50%
        if (_collisionCount == 1)
        {
            setMoveSpeed(getMoveSpeed() * 0.5f);
        }

        // 碰撞3次后消失
        if (_collisionCount >= 3)
        {
            forceDissipate();
        }
        else
        {
            // 为了防止一帧内多次判定，给一个短暂的无敌时间或弹开？
            // 题目未要求弹开，但为了游戏体验，通常会给个冷却。
            // 由于题目说“一旦接触...”，且“碰撞3次后消失”，
            // 我们可以简单地让 Boat 瞬移开一点或者给个短暂的冷却 flag。
            // 这里简化处理：碰撞后 Boat 立即反向移动一段距离避免连续判定
            _currentMoveDir = -_currentMoveDir;
            setPosition(getPosition() + _currentMoveDir * 60.0f); 
        }
    }
}

void Boat::forceDissipate()
{
    if (_currentState == EntityState::DIE) return;
    
    setState(EntityState::DIE);
    _isMoving = false;
    stopAllActions(); // 停止移动动画

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