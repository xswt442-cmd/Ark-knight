#include "Enemy.h"
#include "Entities/Player/Player.h"
#include <cmath>

Enemy::Enemy()
    : _enemyType(EnemyType::MELEE)
    , _sightRange(Constants::Enemy::CHASE_RANGE)
    , _attackRange(Constants::Enemy::ATTACK_RANGE)
    , _patrolTarget(Vec2::ZERO)
    , _patrolTimer(0.0f)
    , _patrolInterval(2.0f)
    , _hasTarget(false)
    , _attackWindup(0.5f) // 默认前摇 0.5 秒
    , _poisonStacks(0)
    , _poisonTimer(0.0f)
    , _poisonTickAcc(0.0f)
    , _poisonSourceAttack(0)
    , _poisonColorSaved(false)
{
}

Enemy::~Enemy()
{
}

bool Enemy::init()
{
    if (!Character::init())
    {
        return false;
    }
    
    // 设置敌人默认属性
    setHP(Constants::Enemy::MELEE_HP);
    setMaxHP(Constants::Enemy::MELEE_HP);
    setMoveSpeed(Constants::Enemy::DEFAULT_MOVE_SPEED);
    setAttack(10);
    
    // 设置Tag
    setTag(Constants::Tag::ENEMY);
    
    return true;
}

void Enemy::update(float dt)
{
    Character::update(dt);
    
    // 处理毒伤逻辑
    if (_poisonStacks > 0)
    {
        // 倒计时
        _poisonTimer -= dt;
        _poisonTickAcc += dt;
        
        // 每 POISON_TICK_INTERVAL 秒造成一次毒伤
        while (_poisonTickAcc >= POISON_TICK_INTERVAL)
        {
            _poisonTickAcc -= POISON_TICK_INTERVAL;
            
            // 计算毒伤：每层每次造成源攻击的 10%
            if (_poisonSourceAttack > 0 && _poisonStacks > 0)
            {
                float dmgF = static_cast<float>(_poisonSourceAttack) * POISON_TICK_RATIO * static_cast<float>(_poisonStacks);
                int dmg = static_cast<int>(std::round(dmgF));
                if (dmg > 0)
                {
                    this->takeDamage(dmg);
                    GAME_LOG("Poison tick: %d damage applied to enemy (stacks=%d, srcAtk=%d)", dmg, _poisonStacks, _poisonSourceAttack);
                }
            }
        }
        
        // 如果倒计时结束，清除毒状态（清空层数并恢复颜色）
        if (_poisonTimer <= 0.0f)
        {
            _poisonStacks = 0;
            _poisonTimer = 0.0f;
            _poisonTickAcc = 0.0f;
            _poisonSourceAttack = 0;
            // 恢复颜色
            if (_poisonColorSaved && _sprite)
            {
                _sprite->setColor(_poisonOriginalColor);
            }
            _poisonColorSaved = false;
            GAME_LOG("Poison expired on enemy, stacks cleared");
        }
    }
}

void Enemy::executeAI(Player* player, float dt)
{
    // 玩家为空、玩家死亡、或自己死亡时不执行AI
    if (player == nullptr || player->isDead() || _currentState == EntityState::DIE)
    {
        return;
    }
    
    // 检测玩家是否在视野范围内
    if (isPlayerInSight(player))
    {
        _hasTarget = true;
        
        // 检测是否在攻击范围内
        if (isPlayerInAttackRange(player))
        {
            // 停止移动，面向玩家，攻击（但延迟造成伤害）
            move(Vec2::ZERO, dt);
            faceToPosition(player->getPosition());
            
            if (canAttack())
            {
                // 立即进入攻击状态并播放攻击前摇动画（子类 attack() 会处理）
                attack();
                
                // 使用成员 _attackWindup 作为风箱时长，风箱结束时再判断是否命中
                auto delay = DelayTime::create(this->_attackWindup);
                auto callback = CallFunc::create([this, player]() {
                    if (player == nullptr || player->isDead() || this->_currentState == EntityState::DIE)
                    {
                        return;
                    }
                    // 只有在玩家仍然处于攻击范围内才造成伤害
                    if (this->isPlayerInAttackRange(player))
                    {
                        // 播放一次用于命中/伤害的动画（每次造成伤害都播放）
                        this->playAttackAnimation();
                        this->attackPlayer(player);
                    }
                    else
                    {
                        GAME_LOG("Player escaped during attack windup");
                    }
                });
                auto seq = Sequence::create(delay, callback, nullptr);
                this->runAction(seq);
            }
        }
        else
        {
            // 追击玩家
            chasePlayer(player, dt);
        }
    }
    else
    {
        _hasTarget = false;
        
        // 巡逻
        patrol(dt);
    }
}

bool Enemy::isPlayerInSight(Player* player) const
{
    if (player == nullptr || player->isDead())
    {
        return false;
    }
    
    float distance = this->getPosition().distance(player->getPosition());
    return distance <= _sightRange;
}

void Enemy::chasePlayer(Player* player, float dt)
{
    if (player == nullptr || player->isDead())
    {
        return;
    }
    
    Vec2 direction = player->getPosition() - this->getPosition();
    move(direction, dt);
    
    // 面向玩家
    faceToPosition(player->getPosition());
}

void Enemy::patrol(float dt)
{
    _patrolTimer += dt;
    
    // 到达巡逻点或超时，选择新的巡逻目标
    if (_patrolTimer >= _patrolInterval || 
        this->getPosition().distance(_patrolTarget) < 10.0f)
    {
        _patrolTimer = 0.0f;
        
        // 随机选择新的巡逻点（当前位置附近）
        float randomX = RANDOM_FLOAT(-100, 100);
        float randomY = RANDOM_FLOAT(-100, 100);
        _patrolTarget = this->getPosition() + Vec2(randomX, randomY);
    }
    
    // 向巡逻目标移动
    Vec2 direction = _patrolTarget - this->getPosition();
    if (direction.lengthSquared() > 1.0f)
    {
        move(direction, dt);
    }
    else
    {
        // 明确告诉 move 停止（以便停止移动动画）
        move(Vec2::ZERO, dt);
    }
}

void Enemy::attack()
{
    if (!canAttack())
    {
        return;
    }
    
    setState(EntityState::ATTACK);
    resetAttackCooldown();
    
    GAME_LOG("Enemy attacks!");
    
    // 攻击动画结束后返回IDLE（子类可以覆盖视觉行为）
    auto delay = DelayTime::create(this->_attackWindup);
    auto callback = CallFunc::create([this]() {
        if (_currentState == EntityState::ATTACK)
        {
            setState(EntityState::IDLE);
        }
    });
    auto sequence = Sequence::create(delay, callback, nullptr);
    this->runAction(sequence);
}

bool Enemy::isPlayerInAttackRange(Player* player) const
{
    if (player == nullptr || player->isDead())
    {
        return false;
    }
    
    float distance = this->getPosition().distance(player->getPosition());
    return distance <= _attackRange;
}

void Enemy::attackPlayer(Player* player)
{
    if (player == nullptr || player->isDead())
    {
        return;
    }
    
    // 造成伤害
    player->takeDamage(_attack);
    
    GAME_LOG("Enemy deals %d damage to player", _attack);
}

void Enemy::playAttackAnimation()
{
    // 默认空实现，子类可覆盖以播放具体动画（不改变冷却/状态）
}

// ==================== Nymph 中毒逻辑实现 ====================
void Enemy::applyNymphPoison(int sourceAttack)
{
    // 保存原始颜色（首次中毒时）
    if (!_poisonColorSaved && _sprite)
    {
        _poisonOriginalColor = _sprite->getColor();
        _poisonColorSaved = true;
    }
    
    // 重置毒计时为 10s
    _poisonTimer = POISON_DURATION;
    _poisonTickAcc = 0.0f;
    
    // 更新毒源攻击力为当前来源攻击力（使用最近一次来源的攻击力）
    _poisonSourceAttack = sourceAttack;
    
    // 叠加一层，最多 POISON_MAX_STACKS
    _poisonStacks = std::min(POISON_MAX_STACKS, _poisonStacks + 1);
    
    // 变色为偏紫
    if (_sprite)
    {
        // 选一个偏紫颜色
        Color3B purple(180, 100, 200);
        _sprite->setColor(purple);
    }
    
    GAME_LOG("applyNymphPoison: stacks=%d, srcAtk=%d", _poisonStacks, _poisonSourceAttack);
}
