#include "Ayao.h"

static const int AYAO_MOVE_ACTION_TAG = 0xA001; // 用于识别移动循环动作的 tag

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
    setMaxHP(50);
    setHP(50);
    setAttack(10);
    setMoveSpeed(100.0f);
    
    // AI参数
    setSightRange(250.0f);      // 视野范围
    setAttackRange(40.0f);      // 攻击范围
    
    // 攻击冷却
    setAttackCooldown(1.5f);    // 1.5秒攻击一次
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
    
    // 播放攻击动画（张嘴-闭嘴完整过程）
    if (_attackAnimation && _sprite)
    {
        // 停止之前的动作（包括移动循环）
        _sprite->stopAllActions();
        
        // 设置动画完成后恢复到第一帧（闭嘴状态）
        _attackAnimation->setRestoreOriginalFrame(true);
        
        auto animate = Animate::create(_attackAnimation);
        auto callback = CallFunc::create([this]() {
            if (_currentState == EntityState::ATTACK)
            {
                setState(EntityState::IDLE);
            }
        });
        auto sequence = Sequence::create(animate, callback, nullptr);
        _sprite->runAction(sequence);
    }
    else
    {
        // 没有动画时的回退逻辑
        auto delay = DelayTime::create(0.5f);
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
 * 注意：此处假设基类 Character::move 会实际更改位置（根据速度与 dt）。
 */
void Ayao::move(const Vec2& direction, float dt)
{
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
        // 左右翻转：修改为正确方向
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
