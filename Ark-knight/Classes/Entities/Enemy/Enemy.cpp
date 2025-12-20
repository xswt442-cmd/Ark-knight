#include "Enemy.h"
#include "Entities/Player/Player.h"

Enemy::Enemy()
    : _enemyType(EnemyType::MELEE)
    , _sightRange(Constants::Enemy::CHASE_RANGE)
    , _attackRange(Constants::Enemy::ATTACK_RANGE)
    , _patrolTarget(Vec2::ZERO)
    , _patrolTimer(0.0f)
    , _patrolInterval(2.0f)
    , _hasTarget(false)
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
    
    // AI逻辑在GameScene中调用，这里不自动执行
}

void Enemy::executeAI(Player* player, float dt)
{
    if (player == nullptr || _currentState == EntityState::DIE)
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
            // 停止移动，面向玩家，攻击
            faceToPosition(player->getPosition());
            
            if (canAttack())
            {
                attack();
                attackPlayer(player);
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
    if (player == nullptr)
    {
        return false;
    }
    
    float distance = this->getPosition().distance(player->getPosition());
    return distance <= _sightRange;
}

void Enemy::chasePlayer(Player* player, float dt)
{
    if (player == nullptr)
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
    
    // 攻击动画结束后返回IDLE
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

bool Enemy::isPlayerInAttackRange(Player* player) const
{
    if (player == nullptr)
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
