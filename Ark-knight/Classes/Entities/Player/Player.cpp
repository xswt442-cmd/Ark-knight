#include "Player.h"

Player::Player()
    : _keyW(false)
    , _keyA(false)
    , _keyS(false)
    , _keyD(false)
    , _keySpace(false)
    , _skillCooldown(Constants::Combat::SKILL_COOLDOWN)
    , _skillCooldownTimer(0.0f)
    , _dashCooldown(2.0f)
    , _dashCooldownTimer(0.0f)
    , _isDashing(false)
    , _armor(0)
    , _maxArmor(50)
    , _keyboardListener(nullptr)
    , _mouseListener(nullptr)
{
}

Player::~Player()
{
    removeInputEvents();
}

bool Player::init()
{
    if (!Character::init())
    {
        return false;
    }
    
    // 设置玩家默认属性
    setHP(Constants::Player::DEFAULT_HP);
    setMaxHP(Constants::Player::DEFAULT_HP);
    setMP(Constants::Player::DEFAULT_MP);
    setMaxMP(Constants::Player::DEFAULT_MP);
    setMoveSpeed(Constants::Player::DEFAULT_MOVE_SPEED);
    setAttack(20);
    
    // 注册输入事件
    registerInputEvents();
    
    // 设置Tag
    setTag(Constants::Tag::PLAYER);
    
    return true;
}

void Player::update(float dt)
{
    Character::update(dt);
    
    // MP自动恢复：每秒回5点
    if (_mp < _maxMP)
    {
        int mpRegenPerSecond = 5;
        _mp += static_cast<int>(mpRegenPerSecond * dt);
        if (_mp > _maxMP)
        {
            _mp = _maxMP;
        }
    }
    
    // 更新技能冷却
    if (_skillCooldownTimer > 0)
    {
        _skillCooldownTimer -= dt;
    }
    
    // 更新冲刺冷却
    if (_dashCooldownTimer > 0)
    {
        _dashCooldownTimer -= dt;
    }
    
    // 处理移动输入
    if (!_isDashing && _currentState != EntityState::DIE)
    {
        handleMoveInput(dt);
    }
}

void Player::registerInputEvents()
{
    // 键盘事件监听
    _keyboardListener = EventListenerKeyboard::create();
    
    _keyboardListener->onKeyPressed = [this](EventKeyboard::KeyCode keyCode, Event* event) {
        switch (keyCode)
        {
            case EventKeyboard::KeyCode::KEY_W:
                _keyW = true;
                break;
            case EventKeyboard::KeyCode::KEY_A:
                _keyA = true;
                break;
            case EventKeyboard::KeyCode::KEY_S:
                _keyS = true;
                break;
            case EventKeyboard::KeyCode::KEY_D:
                _keyD = true;
                break;
            case EventKeyboard::KeyCode::KEY_SPACE:
                _keySpace = true;
                if (canDash())
                {
                    dash();
                }
                break;
            case EventKeyboard::KeyCode::KEY_J:
                // 普通攻击
                if (canAttack())
                {
                    attack();
                }
                break;
            case EventKeyboard::KeyCode::KEY_K:
                // 释放技能
                if (canUseSkill())
                {
                    useSkill();
                }
                break;
            default:
                break;
        }
    };
    
    _keyboardListener->onKeyReleased = [this](EventKeyboard::KeyCode keyCode, Event* event) {
        switch (keyCode)
        {
            case EventKeyboard::KeyCode::KEY_W:
                _keyW = false;
                break;
            case EventKeyboard::KeyCode::KEY_A:
                _keyA = false;
                break;
            case EventKeyboard::KeyCode::KEY_S:
                _keyS = false;
                break;
            case EventKeyboard::KeyCode::KEY_D:
                _keyD = false;
                break;
            case EventKeyboard::KeyCode::KEY_SPACE:
                _keySpace = false;
                break;
            default:
                break;
        }
    };
    
    _eventDispatcher->addEventListenerWithSceneGraphPriority(_keyboardListener, this);
    
    // 鼠标事件监听
    _mouseListener = EventListenerMouse::create();
    
    _mouseListener->onMouseDown = [this](Event* event) {
        EventMouse* mouseEvent = static_cast<EventMouse*>(event);
        if (mouseEvent->getMouseButton() == EventMouse::MouseButton::BUTTON_LEFT)
        {
            // 左键攻击
            if (canAttack())
            {
                attack();
            }
        }
        else if (mouseEvent->getMouseButton() == EventMouse::MouseButton::BUTTON_RIGHT)
        {
            // 右键技能
            if (canUseSkill())
            {
                useSkill();
            }
        }
    };
    
    _eventDispatcher->addEventListenerWithSceneGraphPriority(_mouseListener, this);
    
    GAME_LOG("Player input events registered");
}

void Player::removeInputEvents()
{
    if (_keyboardListener != nullptr)
    {
        _eventDispatcher->removeEventListener(_keyboardListener);
        _keyboardListener = nullptr;
    }
    
    if (_mouseListener != nullptr)
    {
        _eventDispatcher->removeEventListener(_mouseListener);
        _mouseListener = nullptr;
    }
}

void Player::handleMoveInput(float dt)
{
    Vec2 moveDir = Vec2::ZERO;
    
    if (_keyW) moveDir.y += 1;
    if (_keyS) moveDir.y -= 1;
    if (_keyA) moveDir.x -= 1;
    if (_keyD) moveDir.x += 1;
    
    // 移动
    move(moveDir, dt);
}

bool Player::canUseSkill() const
{
    return _skillCooldownTimer <= 0 
        && _currentState != EntityState::DIE
        && _mp >= getSkillMPCost();
}

void Player::resetSkillCooldown()
{
    _skillCooldownTimer = _skillCooldown;
}

float Player::getSkillCooldownRemaining() const
{
    return _skillCooldownTimer > 0.0f ? _skillCooldownTimer : 0.0f;
}

void Player::dash()
{
    if (!canDash())
    {
        return;
    }
    
    _isDashing = true;
    setState(EntityState::DASH);
    
    // 计算冲刺方向
    Vec2 dashDir = _facingDirection;
    if (dashDir.lengthSquared() < 0.01f)
    {
        dashDir = Vec2(1, 0);  // 默认向右
    }
    
    // 冲刺距离
    float dashDistance = Constants::Player::DASH_SPEED * Constants::Player::DASH_DURATION;
    Vec2 targetPos = this->getPosition() + dashDir * dashDistance;
    
    // 执行冲刺动作
    auto moveTo = MoveTo::create(Constants::Player::DASH_DURATION, targetPos);
    auto callback = CallFunc::create([this]() {
        _isDashing = false;
        setState(EntityState::IDLE);
    });
    auto sequence = Sequence::create(moveTo, callback, nullptr);
    
    this->runAction(sequence);
    
    // 重置冲刺冷却
    _dashCooldownTimer = _dashCooldown;
    
    GAME_LOG("Player dash!");
}

bool Player::canDash() const
{
    return _dashCooldownTimer <= 0 
        && !_isDashing 
        && _currentState != EntityState::DIE;
}

void Player::attack()
{
    if (!canAttack())
    {
        return;
    }
    
    setState(EntityState::ATTACK);
    resetAttackCooldown();
    
    GAME_LOG("Player attacks!");
    
    // 显示攻击范围特效
    if (this->getParent() != nullptr)
    {
        auto attackEffect = DrawNode::create();
        // 绘制扇形攻击范围
        Vec2 attackPos = this->getPosition() + _facingDirection * 40;
        attackEffect->drawSolidCircle(attackPos, 50.0f, 0, 16, Color4F(1.0f, 1.0f, 0.0f, 0.5f));
        this->getParent()->addChild(attackEffect, Constants::ZOrder::EFFECT);
        
        // 攻击特效快速消失
        auto fadeOut = FadeOut::create(0.2f);
        auto remove = RemoveSelf::create();
        attackEffect->runAction(Sequence::create(fadeOut, remove, nullptr));
    }
    
    // 攻击动画结束后返回IDLE
    auto delay = DelayTime::create(0.3f);
    auto callback = CallFunc::create([this]() {
        if (_currentState == EntityState::ATTACK)
        {
            setState(EntityState::IDLE);
        }
    });
    auto sequence = Sequence::create(delay, callback, nullptr);
    this->runAction(sequence);
}

void Player::takeDamage(int damage)
{
    // 先扣除护甲
    if (_armor > 0)
    {
        int armorDamage = std::min(_armor, damage);
        _armor -= armorDamage;
        damage -= armorDamage;
        
        GAME_LOG("Armor absorbed %d damage, remaining armor: %d", armorDamage, _armor);
    }
    
    // 剩余伤害扣血
    if (damage > 0)
    {
        Character::takeDamage(damage);
    }
    
    // 进入受击状态
    if (_currentState != EntityState::DIE)
    {
        setState(EntityState::HIT);
        
        // 短暂硬直后恢复
        auto delay = DelayTime::create(0.2f);
        auto callback = CallFunc::create([this]() {
            if (_currentState == EntityState::HIT)
            {
                setState(EntityState::IDLE);
            }
        });
        auto sequence = Sequence::create(delay, callback, nullptr);
        this->runAction(sequence);
    }
}
