#include "Ayao.h"

static const int AYAO_MOVE_ACTION_TAG = 0xA001; // 移动循环动作 tag
static const int AYAO_HIT_ACTION_TAG  = 0xA002; // 命中/伤害播放动作 tag
static const int AYAO_WINDUP_ACTION_TAG = 0xA003; // 攻击前摇动作 tag

Ayao::Ayao()
    : _moveAnimation(nullptr)
    , _attackAnimation(nullptr)
    , _dieAnimation(nullptr)
    , _roomBounds(Rect::ZERO)
    , _hasRoomBounds(false)
{
}

Ayao::~Ayao()
{
    CC_SAFE_RELEASE(_moveAnimation);
    CC_SAFE_RELEASE(_attackAnimation);
    CC_SAFE_RELEASE(_dieAnimation);
}

bool Ayao::init()
{
    if (!Enemy::init())
    {
        return false;
    }
    
    // 设置敌人类型
    setEnemyType(EnemyType::MELEE);
    
    // 设置属性
    setupAyaoAttributes();
    
    // 加载动画
    loadAnimations();
    
    GAME_LOG("Ayao enemy created");
    
    return true;
}

void Ayao::update(float dt)
{
    // 死亡后不更新
    if (_currentState == EntityState::DIE)
    {
        return;
    }
    
    Enemy::update(dt);
    
    // 限制位置在房间边界内
    if (_hasRoomBounds)
    {
        Vec2 pos = this->getPosition();
        pos.x = std::max(_roomBounds.getMinX(), std::min(pos.x, _roomBounds.getMaxX()));
        pos.y = std::max(_roomBounds.getMinY(), std::min(pos.y, _roomBounds.getMaxY()));
        this->setPosition(pos);
    }
}

void Ayao::setupAyaoAttributes()
{
    // 阿咬基础属性
    setMaxHP(100);
    setHP(100);
    setAttack(10);
    setMoveSpeed(100.0f);
    
    // AI参数
    setSightRange(250.0f);      // 视野范围
    setAttackRange(40.0f);      // 攻击范围
    
    // 攻击冷却
    setAttackCooldown(1.5f);    // 1.5秒攻击一次

    // 可按怪物个体设置不同前摇时长
    setAttackWindup(0.5f);
}

void Ayao::loadAnimations()
{
    // ==================== 移动动画 ====================
    Vector<SpriteFrame*> moveFrames;
    for (int i = 1; i <= 5; i++)
    {
        char filename[128];
        sprintf(filename, "Enemy/AYao/AYao_Move/AYao_Move_%04d.png", i);
        auto frame = Sprite::create(filename)->getSpriteFrame();
        if (frame)
        {
            moveFrames.pushBack(frame);
        }
    }
    
    if (!moveFrames.empty())
    {
        _moveAnimation = Animation::createWithSpriteFrames(moveFrames, 0.1f);
        _moveAnimation->retain();
    }
    
    // ==================== 攻击动画 ====================
    Vector<SpriteFrame*> attackFrames;
    for (int i = 1; i <= 5; i++)
    {
        char filename[128];
        sprintf(filename, "Enemy/AYao/AYao_Attack/AYao_Attack_%04d.png", i);
        auto frame = Sprite::create(filename);
        if (frame)
        {
            attackFrames.pushBack(frame->getSpriteFrame());
        }
    }
    
    if (!attackFrames.empty())
    {
        _attackAnimation = Animation::createWithSpriteFrames(attackFrames, 0.1f);
        _attackAnimation->retain();
    }
    
    // ==================== 死亡动画 ====================
    Vector<SpriteFrame*> dieFrames;
    for (int i = 1; i <= 5; i++)  // 加载全部5帧
    {
        char filename[128];
        sprintf(filename, "Enemy/AYao/AYao_Die/AYao_Die_%04d.png", i);
        auto frame = Sprite::create(filename);
        if (frame)
        {
            dieFrames.pushBack(frame->getSpriteFrame());
        }
    }
    
    if (!dieFrames.empty())
    {
        _dieAnimation = Animation::createWithSpriteFrames(dieFrames, 0.12f);  // 每帧0.12秒
        _dieAnimation->retain();
    }
    
    // 设置初始精灵（使用移动动画第一帧）
    if (_moveAnimation)
    {
        auto frames = _moveAnimation->getFrames();
        if (!frames.empty())
        {
            auto sprite = Sprite::createWithSpriteFrame(frames.front()->getSpriteFrame());
            this->bindSprite(sprite);
        }
    }
    
    GAME_LOG("Ayao animations loaded");
}

void Ayao::attack()
{
    if (!canAttack())
    {
        return;
    }
    
    setState(EntityState::ATTACK);
    resetAttackCooldown();
    
    // 播放攻击前摇动画（只停止移动循环，不停止其它潜在动作）
    if (_attackAnimation && _sprite)
    {
        // 仅停止移动循环动作（避免 stopAllActions 导致其它视觉动作被中断）
        auto moveAct = _sprite->getActionByTag(AYAO_MOVE_ACTION_TAG);
        if (moveAct)
        {
            _sprite->stopAction(moveAct);
        }

        // 停止之前的前摇动作（如果有）
        auto prevWind = _sprite->getActionByTag(AYAO_WINDUP_ACTION_TAG);
        if (prevWind)
        {
            _sprite->stopAction(prevWind);
        }

        // 作为前摇播放攻击动画（当风箱结束时，AI 的回调会决定是否命中）
        _attackAnimation->setRestoreOriginalFrame(true);
        auto animate = Animate::create(_attackAnimation);
        animate->setTag(AYAO_WINDUP_ACTION_TAG);
        _sprite->runAction(animate);

        // 使用 Enemy 中的 windup 时长保证状态同步（结束后回到 IDLE）
        float windup = this->getAttackWindup();
        auto delay = DelayTime::create(windup);
        auto callback = CallFunc::create([this]() {
            if (_currentState == EntityState::ATTACK)
            {
                setState(EntityState::IDLE);
            }
        });
        auto seq = Sequence::create(delay, callback, nullptr);
        this->runAction(seq); // 放在节点上，避免和 sprite 上动画冲突
    }
    else
    {
        // 没有动画时使用 windup 时长回退（避免硬编码）
        float windup = this->getAttackWindup();
        auto delay = DelayTime::create(windup);
        auto callback = CallFunc::create([this]() {
            if (_currentState == EntityState::ATTACK)
            {
                setState(EntityState::IDLE);
            }
        });
        auto sequence = Sequence::create(delay, callback, nullptr);
        this->runAction(sequence);
    }
    
    GAME_LOG("Ayao attacks!");
}

void Ayao::die()
{
    // 防止重复调用
    if (_currentState == EntityState::DIE)
    {
        GAME_LOG("Ayao::die() - already dead, skipping");
        return;
    }
    
    setState(EntityState::DIE);
    _isAlive = false;  // 标记为不存活
    
    // 停止所有动作
    this->stopAllActions();
    if (_sprite)
    {
        _sprite->stopAllActions();
        _sprite->setVisible(true);  // 确保精灵可见
        _sprite->setOpacity(255);   // 确保不透明
    }
    
    GAME_LOG("Ayao died - _dieAnimation: %p, _sprite: %p", _dieAnimation, _sprite);
    
    // 播放死亡动画
    if (_dieAnimation && _sprite)
    {
        GAME_LOG("Playing Ayao death animation, frames: %zd, duration: %.2f", 
                 _dieAnimation->getFrames().size(), _dieAnimation->getDuration());
        
        // 播放死亡动画
        auto animate = Animate::create(_dieAnimation);
        
        // 动画播放完毕后淡出并移除节点
        auto fadeOut = FadeOut::create(0.5f);
        auto removeCallback = CallFunc::create([this]() {
            GAME_LOG("Ayao death animation finished, removing from parent");
            this->removeFromParent();
        });
        auto sequence = Sequence::create(animate, fadeOut, removeCallback, nullptr);
        _sprite->runAction(sequence);
    }
    else
    {
        GAME_LOG("Ayao death - no animation or sprite, direct remove");
        // 没有动画时直接淡出移除
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

/**
 * 重写移动：控制移动动画开始/停止，以及根据水平分量设置左右朝向（flipX）。
 * 注意：保留你已设置的朝向判断，不做修改。
 */
void Ayao::move(const Vec2& direction, float dt)
{
    // 在攻击阶段禁止移动（确保攻击前摇/命中动画不会被移动覆盖）
    if (_currentState == EntityState::ATTACK || _currentState == EntityState::DIE)
    {
        // 明确告诉基类停止逻辑（避免位置变化），直接返回
        Character::move(Vec2::ZERO, dt);
        return;
    }

    // 小阈值判断为“不移动”
    const float STOP_THRESHOLD_SQ = 1.0f;
    if (direction.lengthSquared() <= STOP_THRESHOLD_SQ)
    {
        // 停止移动动作（如果存在）
        if (_sprite)
        {
            auto act = _sprite->getActionByTag(AYAO_MOVE_ACTION_TAG);
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
        
        // 调用基类移动以保持状态一致（基类可能处理速度、状态机等）
        Character::move(Vec2::ZERO, dt);
        return;
    }
    
    // 有移动向量：先调用基类以实际移动实体
    Vec2 dirNorm = direction.getNormalized();
    Character::move(dirNorm, dt);
    
    // 设置朝向（左右），并确保移动动画在循环播放
    if (_sprite && _moveAnimation)
    {
        // 保持你现有的朝向逻辑（不要改）
        _sprite->setFlippedX(dirNorm.x > 0.0f);
        
        // 如果移动动画未在播放，则启动循环播放并打上 tag
        if (!_sprite->getActionByTag(AYAO_MOVE_ACTION_TAG))
        {
            auto animate = Animate::create(_moveAnimation);
            auto repeat = RepeatForever::create(animate);
            repeat->setTag(AYAO_MOVE_ACTION_TAG);
            _sprite->runAction(repeat);
        }
    }
}

/**
 * 每次真正造成伤害时播放一次命中/攻击动画（不改变冷却/状态）
 */
void Ayao::playAttackAnimation()
{
    if (!_sprite)
    {
        return;
    }

    // 如果已经死亡则忽略
    if (_currentState == EntityState::DIE)
    {
        return;
    }
    
    // 在播放命中动画期间确保处于 ATTACK 状态，阻止移动
    if (_currentState != EntityState::ATTACK)
    {
        setState(EntityState::ATTACK);
    }
    
    // 停止当前的移动循环动作（保持其他动作不被影响）
    auto moveAct = _sprite->getActionByTag(AYAO_MOVE_ACTION_TAG);
    if (moveAct)
    {
        _sprite->stopAction(moveAct);
    }
    
    // 如果前摇动画还在播放，停止它（命中动画优先）
    auto wind = _sprite->getActionByTag(AYAO_WINDUP_ACTION_TAG);
    if (wind)
    {
        _sprite->stopAction(wind);
    }
    
    if (_attackAnimation)
    {
        // 如果之前有命中动作在播，先停止它
        auto prevHit = _sprite->getActionByTag(AYAO_HIT_ACTION_TAG);
        if (prevHit)
        {
            _sprite->stopAction(prevHit);
        }
        
        _attackAnimation->setRestoreOriginalFrame(true);
        auto animate = Animate::create(_attackAnimation);
        animate->setTag(AYAO_HIT_ACTION_TAG);
        
        // 播放后恢复到移动动画的第一帧（或保持当前帧），并在结束后恢复状态为 IDLE（如果未死亡）
        auto callback = CallFunc::create([this]() {
            if (_moveAnimation && _sprite)
            {
                auto frames = _moveAnimation->getFrames();
                if (!frames.empty())
                {
                    _sprite->setSpriteFrame(frames.front()->getSpriteFrame());
                }
            }
            // 命中动画播放完毕后恢复状态（前提不是死亡）
            if (_currentState != EntityState::DIE)
            {
                setState(EntityState::IDLE);
            }
        });
        auto seq = Sequence::create(animate, callback, nullptr);
        seq->setTag(AYAO_HIT_ACTION_TAG);
        _sprite->runAction(seq);
    }
}
