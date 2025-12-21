#include "Character.h"

Character::Character()
    : _currentState(EntityState::IDLE)
    , _moveSpeed(Constants::Player::DEFAULT_MOVE_SPEED)
    , _attack(10)
    , _attackCooldown(Constants::Combat::ATTACK_COOLDOWN)
    , _attackCooldownTimer(0.0f)
    , _mp(Constants::Player::DEFAULT_MP)
    , _maxMP(Constants::Player::DEFAULT_MP)
    , _facingDirection(Vec2(1, 0))
{
}

Character::~Character()
{
}

bool Character::init()
{
    if (!GameEntity::init())
    {
        return false;
    }
    
    return true;
}

void Character::update(float dt)
{
    GameEntity::update(dt);
    
    // 更新状态机
    updateStateMachine(dt);
    
    // 更新攻击冷却
    if (_attackCooldownTimer > 0)
    {
        _attackCooldownTimer -= dt;
    }
}

void Character::setState(EntityState state)
{
    if (_currentState == state)
    {
        return;
    }
    
    // 状态退出逻辑
    switch (_currentState)
    {
        case EntityState::IDLE:
            break;
        case EntityState::MOVE:
            break;
        case EntityState::ATTACK:
            break;
        case EntityState::SKILL:
            break;
        case EntityState::HIT:
            break;
        case EntityState::DASH:
            break;
        case EntityState::DIE:
            break;
    }
    
    // 切换状态
    _currentState = state;
    
    // 状态进入逻辑
    switch (_currentState)
    {
        case EntityState::IDLE:
            GAME_LOG("Character enter IDLE state");
            break;
        case EntityState::MOVE:
            GAME_LOG("Character enter MOVE state");
            break;
        case EntityState::ATTACK:
            GAME_LOG("Character enter ATTACK state");
            break;
        case EntityState::SKILL:
            GAME_LOG("Character enter SKILL state");
            break;
        case EntityState::HIT:
            GAME_LOG("Character enter HIT state");
            break;
        case EntityState::DASH:
            GAME_LOG("Character enter DASH state");
            break;
        case EntityState::DIE:
            GAME_LOG("Character enter DIE state");
            break;
    }
}

void Character::updateStateMachine(float dt)
{
    switch (_currentState)
    {
        case EntityState::IDLE:
            // Idle状态逻辑
            break;
            
        case EntityState::MOVE:
            // Move状态逻辑
            break;
            
        case EntityState::ATTACK:
            // Attack状态逻辑
            break;
            
        case EntityState::SKILL:
            // Skill状态逻辑
            break;
            
        case EntityState::HIT:
            // Hit状态逻辑
            break;
            
        case EntityState::DASH:
            // Dash状态逻辑
            break;
            
        case EntityState::DIE:
            // Die状态逻辑
            break;
    }
}

void Character::move(const Vec2& direction, float dt)
{
    if (direction.lengthSquared() < 0.01f)
    {
        // 没有移动输入
        if (_currentState == EntityState::MOVE)
        {
            setState(EntityState::IDLE);
        }
        return;
    }
    
    // 归一化方向
    Vec2 normalizedDir = direction.getNormalized();
    
    // 计算新位置
    Vec2 newPos = this->getPosition() + normalizedDir * _moveSpeed * dt;
    
    // 移除了旧的屏幕边界限制，现在由地图系统的Room和Hallway来处理边界
    
    this->setPosition(newPos);
    
    // 更新面朝方向
    _facingDirection = normalizedDir;
    
    // 更新状态
    if (_currentState == EntityState::IDLE)
    {
        setState(EntityState::MOVE);
    }
}

void Character::faceToPosition(const Vec2& targetPos)
{
    Vec2 direction = targetPos - this->getPosition();
    if (direction.lengthSquared() > 0.01f)
    {
        _facingDirection = direction.getNormalized();
        
        // 翻转精灵
        if (_sprite != nullptr)
        {
            if (_facingDirection.x < 0)
            {
                _sprite->setFlippedX(true);
            }
            else if (_facingDirection.x > 0)
            {
                _sprite->setFlippedX(false);
            }
        }
    }
}

bool Character::canAttack() const
{
    return _attackCooldownTimer <= 0 && _currentState != EntityState::DIE;
}

void Character::resetAttackCooldown()
{
    _attackCooldownTimer = _attackCooldown;
}

bool Character::consumeMP(int cost)
{
    if (_mp >= cost)
    {
        _mp -= cost;
        GAME_LOG("Consume %d MP, remaining: %d/%d", cost, _mp, _maxMP);
        return true;
    }
    
    GAME_LOG("Not enough MP! Need %d, have %d", cost, _mp);
    return false;
}

void Character::die()
{
    setState(EntityState::DIE);
    
    GAME_LOG("Character died");
    
    // 播放死亡效果
    showDeathEffect();
}
