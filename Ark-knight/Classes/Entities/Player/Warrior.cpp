#include "Warrior.h"
#include "cocos2d.h"
#include "Entities/Enemy/Enemy.h"
#include "Map/Room.h"
#include "Map/Hallway.h"
#include "UI/FloatingText.h"

USING_NS_CC;

Warrior::Warrior()
    : _isEnhanced(false)
    , _enhancedTimer(0.0f)
    , _enhancedDuration(8.0f)
    , _baseAttackInterval(0.8f)
    , _baseAttackRange(80.0f)
    , _attackTimer(0.0f)
    , _currentAnimName("")
{
}

Warrior::~Warrior()
{
    // 释放动画缓存
    for (auto& pair : _animations)
    {
        if (pair.second)
        {
            pair.second->release();
        }
    }
    _animations.clear();
}

bool Warrior::init()
{
    if (!Player::init())
    {
        return false;
    }
    
    // 设置泥岩属性（高血量，低蓝）
    setMaxHP(150000);
    setHP(150000);
    setMaxMP(80);
    setMP(80);
    setAttack(300);
    setMoveSpeed(140.0f);  // 战士移速较慢
    
    // 设置技能冷却
    _skillCooldown = 18.0f;
    
    // 设置攻击冷却
    _attackCooldown = _baseAttackInterval;
    
    // 初始化动画
    initAnimations();
    
    // 创建精灵并绑定
    auto sprite = Sprite::create("Player/Mudrock/MudRock_Idle/MudRock_Idle_0001.png");
    if (sprite)
    {
        // 设置缩放（战士稍微大一点）
        float targetSize = Constants::FLOOR_TILE_SIZE * 4.5f;
        float scale = targetSize / sprite->getContentSize().height;
        sprite->setScale(scale);
        
        bindSprite(sprite, Constants::ZOrder::ENTITY);
        
        // 播放默认待机动画
        playAnimation("idle", true);
    }
    
    GAME_LOG("Mudrock(Warrior) initialized - HP: %d/%d, MP: %d/%d", 
             getHP(), getMaxHP(), getMP(), getMaxMP());
    
    return true;
}

void Warrior::initAnimations()
{
    float frameDelay = 0.12f;  // 战士动画稍慢一些
    
    // Idle动画 - 4帧
    {
        Vector<SpriteFrame*> frames;
        for (int i = 1; i <= 4; i++)
        {
            char filename[128];
            sprintf(filename, "Player/Mudrock/MudRock_Idle/MudRock_Idle_%04d.png", i);
            auto sprite = Sprite::create(filename);
            if (sprite)
            {
                frames.pushBack(sprite->getSpriteFrame());
            }
        }
        if (!frames.empty())
        {
            auto anim = Animation::createWithSpriteFrames(frames, frameDelay);
            anim->retain();
            _animations["idle"] = anim;
        }
    }
    
    // Move动画 - 4帧
    {
        Vector<SpriteFrame*> frames;
        for (int i = 1; i <= 4; i++)
        {
            char filename[128];
            sprintf(filename, "Player/Mudrock/MudRock_Move/MudRock_Move_%04d.png", i);
            auto sprite = Sprite::create(filename);
            if (sprite)
            {
                frames.pushBack(sprite->getSpriteFrame());
            }
        }
        if (!frames.empty())
        {
            auto anim = Animation::createWithSpriteFrames(frames, frameDelay);
            anim->retain();
            _animations["move"] = anim;
        }
    }
    
    // Attack动画 - 5帧
    {
        Vector<SpriteFrame*> frames;
        for (int i = 1; i <= 5; i++)
        {
            char filename[128];
            sprintf(filename, "Player/Mudrock/MudRock_Attack/MudRock_Attack_%04d.png", i);
            auto sprite = Sprite::create(filename);
            if (sprite)
            {
                frames.pushBack(sprite->getSpriteFrame());
            }
        }
        if (!frames.empty())
        {
            auto anim = Animation::createWithSpriteFrames(frames, 0.08f);  // 攻击动画快一些
            anim->retain();
            _animations["attack"] = anim;
        }
    }
    
    // Die动画 - 5帧
    {
        Vector<SpriteFrame*> frames;
        for (int i = 1; i <= 5; i++)
        {
            char filename[128];
            sprintf(filename, "Player/Mudrock/MudRock_Die/MudRock_Die_%04d.png", i);
            auto sprite = Sprite::create(filename);
            if (sprite)
            {
                frames.pushBack(sprite->getSpriteFrame());
            }
        }
        if (!frames.empty())
        {
            auto anim = Animation::createWithSpriteFrames(frames, frameDelay * 1.5f);
            anim->retain();
            _animations["die"] = anim;
        }
    }
    
    // Skill_Idle动画 - 4帧
    {
        Vector<SpriteFrame*> frames;
        for (int i = 1; i <= 4; i++)
        {
            char filename[128];
            sprintf(filename, "Player/Mudrock/MudRock_Skill_Idle/MudRock_Skill_Idle_%04d.png", i);
            auto sprite = Sprite::create(filename);
            if (sprite)
            {
                frames.pushBack(sprite->getSpriteFrame());
            }
        }
        if (!frames.empty())
        {
            auto anim = Animation::createWithSpriteFrames(frames, frameDelay);
            anim->retain();
            _animations["skill_idle"] = anim;
        }
    }
    
    // Skill_Move动画 - 4帧 (注意文件名中SKill大小写)
    {
        Vector<SpriteFrame*> frames;
        for (int i = 1; i <= 4; i++)
        {
            char filename[128];
            sprintf(filename, "Player/Mudrock/MudRock_Skill_Move/MudRock_SKill_Move_%04d.png", i);
            auto sprite = Sprite::create(filename);
            if (sprite)
            {
                frames.pushBack(sprite->getSpriteFrame());
            }
        }
        if (!frames.empty())
        {
            auto anim = Animation::createWithSpriteFrames(frames, frameDelay);
            anim->retain();
            _animations["skill_move"] = anim;
        }
    }
    
    // Skill_Attack动画 - 5帧 (注意文件名是Attack_Skill)
    {
        Vector<SpriteFrame*> frames;
        for (int i = 1; i <= 5; i++)
        {
            char filename[128];
            sprintf(filename, "Player/Mudrock/MudRock_Skill_Attack/MudRock_Attack_Skill_%04d.png", i);
            auto sprite = Sprite::create(filename);
            if (sprite)
            {
                frames.pushBack(sprite->getSpriteFrame());
            }
        }
        if (!frames.empty())
        {
            auto anim = Animation::createWithSpriteFrames(frames, 0.08f);
            anim->retain();
            _animations["skill_attack"] = anim;
        }
    }
    
    GAME_LOG("Mudrock animations loaded: %zu animations", _animations.size());
}

void Warrior::playAnimation(const std::string& name, bool loop)
{
    if (_sprite == nullptr)
    {
        return;
    }
    
    if (_currentAnimName == name)
    {
        return;
    }
    
    auto it = _animations.find(name);
    if (it == _animations.end() || it->second == nullptr)
    {
        GAME_LOG("Animation not found: %s", name.c_str());
        return;
    }
    
    _sprite->stopAllActions();
    _currentAnimName = name;
    
    auto animate = Animate::create(it->second);
    if (loop)
    {
        auto repeat = RepeatForever::create(animate);
        _sprite->runAction(repeat);
    }
    else
    {
        _sprite->runAction(animate);
    }
}

void Warrior::setState(EntityState state)
{
    EntityState oldState = _currentState;
    Character::setState(state);
    
    switch (state)
    {
        case EntityState::IDLE:
            if (_isEnhanced)
            {
                playAnimation("skill_idle", true);
            }
            else
            {
                playAnimation("idle", true);
            }
            break;
            
        case EntityState::MOVE:
            if (_isEnhanced)
            {
                playAnimation("skill_move", true);
            }
            else
            {
                playAnimation("move", true);
            }
            break;
            
        case EntityState::ATTACK:
            if (_isEnhanced)
            {
                playAnimation("skill_attack", false);
            }
            else
            {
                playAnimation("attack", false);
            }
            break;
            
        case EntityState::DIE:
            playAnimation("die", false);
            break;
            
        default:
            break;
    }
}

void Warrior::update(float dt)
{
    Player::update(dt);
    
    // 更新强化状态计时器
    if (_isEnhanced)
    {
        _enhancedTimer -= dt;
        if (_enhancedTimer <= 0.0f)
        {
            exitEnhancedState();
        }
    }
}

void Warrior::attack()
{
    if (!canAttack())
    {
        return;
    }
    
    setState(EntityState::ATTACK);
    resetAttackCooldown();
    
    // 执行近战范围攻击
    performMeleeAttack();
    
    GAME_LOG("Mudrock melee attack! Enhanced: %s", _isEnhanced ? "YES" : "NO");
    
    // 攻击动画结束后返回IDLE
    auto delay = DelayTime::create(0.4f);
    auto callback = CallFunc::create([this]() {
        if (_currentState == EntityState::ATTACK && !isDead())
        {
            setState(EntityState::IDLE);
        }
    });
    auto sequence = Sequence::create(delay, callback, nullptr);
    this->runAction(sequence);
}

void Warrior::useSkill()
{
    if (!consumeMP(getSkillMPCost()))
    {
        GAME_LOG("Not enough MP to use skill!");
        return;
    }
    
    setState(EntityState::SKILL);
    
    // 恢复20%生命值
    int healAmount = static_cast<int>(getMaxHP() * 0.2f);
    heal(healAmount);
    
    // 显示治疗数字
    if (this->getParent())
    {
        FloatingText::show(this->getParent(), this->getPosition(), 
                          "+" + std::to_string(healAmount), Color3B(0, 255, 100));
    }
    
    // 进入强化状态
    enterEnhancedState();
    
    // 重置技能冷却
    resetSkillCooldown();
    
    GAME_LOG("Mudrock uses skill! Healed: %d, Duration: %.1fs, MP: %d/%d", 
             healAmount, _enhancedDuration, getMP(), getMaxMP());
    
    // 技能动画结束后返回IDLE
    auto delay = DelayTime::create(0.5f);
    auto callback = CallFunc::create([this]() {
        if (getState() == EntityState::SKILL && !isDead())
        {
            setState(EntityState::IDLE);
        }
    });
    auto sequence = Sequence::create(delay, callback, nullptr);
    this->runAction(sequence);
}

void Warrior::enterEnhancedState()
{
    _isEnhanced = true;
    _enhancedTimer = _enhancedDuration;
    
    // 强化状态：攻速略微提升
    _attackCooldown = _baseAttackInterval * 0.7f;
    
    _currentAnimName = "";
    if (_currentState == EntityState::IDLE)
    {
        playAnimation("skill_idle", true);
    }
    else if (_currentState == EntityState::MOVE)
    {
        playAnimation("skill_move", true);
    }
    
    GAME_LOG("Entered enhanced state - Attack interval: %.2fs", _attackCooldown);
}

void Warrior::exitEnhancedState()
{
    _isEnhanced = false;
    _enhancedTimer = 0.0f;
    
    _attackCooldown = _baseAttackInterval;
    
    _currentAnimName = "";
    if (_currentState == EntityState::IDLE)
    {
        playAnimation("idle", true);
    }
    else if (_currentState == EntityState::MOVE)
    {
        playAnimation("move", true);
    }
    
    GAME_LOG("Exited enhanced state - Attack interval: %.2fs", _attackCooldown);
}

void Warrior::performMeleeAttack()
{
    if (this->getParent() == nullptr)
    {
        return;
    }
    
    auto parent = this->getParent();
    Vec2 playerPos = this->getPosition();
    
    // 计算攻击范围和伤害
    float attackRange = _isEnhanced ? _baseAttackRange * 1.5f : _baseAttackRange;
    int damage = _isEnhanced ? getAttack() * 3 : getAttack();  // 强化200%攻击
    
    // 攻击方向（面朝方向的扇形范围）
    Vec2 attackDir = _facingDirection;
    float attackAngle = 120.0f;  // 攻击扇形角度（度）
    
    // 先收集所有符合条件的敌人，避免遍历时修改容器
    std::vector<Enemy*> enemiesToHit;
    for (auto child : parent->getChildren())
    {
        if (child->getTag() == Constants::Tag::ENEMY)
        {
            auto enemy = dynamic_cast<Enemy*>(child);
            if (enemy == nullptr || enemy->isDead() || enemy->isStealthed())
            {
                continue;
            }
            
            Vec2 enemyPos = enemy->getPosition();
            Vec2 toEnemy = enemyPos - playerPos;
            float distance = toEnemy.length();
            
            // 检查距离
            if (distance > attackRange)
            {
                continue;
            }
            
            // 检查角度（是否在攻击扇形内）
            if (distance > 0)
            {
                toEnemy.normalize();
                float dot = attackDir.dot(toEnemy);
                float angleToEnemy = CC_RADIANS_TO_DEGREES(acos(dot));
                
                if (angleToEnemy > attackAngle / 2.0f)
                {
                    continue;
                }
            }
            
            enemiesToHit.push_back(enemy);
        }
    }
    
    // 对收集到的敌人造成伤害
    for (auto enemy : enemiesToHit)
    {
        if (enemy && !enemy->isDead())
        {
            Vec2 enemyPos = enemy->getPosition();
            int oldHP = enemy->getHP();
            enemy->takeDamage(damage);
            int applied = oldHP - enemy->getHP();
            
            if (applied > 0)
            {
                FloatingText::show(parent, enemyPos, std::to_string(applied), Color3B(255, 150, 0));
            }
            
            GAME_LOG("Melee hit enemy for %d damage!", applied);
        }
    }
    
    // 显示攻击范围视觉效果（可选）
    auto attackEffect = DrawNode::create();
    attackEffect->setPosition(playerPos);
    attackEffect->setGlobalZOrder(Constants::ZOrder::ENTITY - 1);
    
    // 绘制扇形攻击范围
    std::vector<Vec2> points;
    points.push_back(Vec2::ZERO);
    int segments = 12;
    float startAngle = CC_RADIANS_TO_DEGREES(atan2(attackDir.y, attackDir.x)) - attackAngle / 2.0f;
    for (int i = 0; i <= segments; i++)
    {
        float angle = CC_DEGREES_TO_RADIANS(startAngle + (attackAngle * i / segments));
        points.push_back(Vec2(cos(angle) * attackRange, sin(angle) * attackRange));
    }
    
    attackEffect->drawPolygon(points.data(), points.size(), 
                              Color4F(1.0f, 0.5f, 0.0f, 0.3f), 
                              2.0f, Color4F(1.0f, 0.7f, 0.0f, 0.6f));
    
    parent->addChild(attackEffect);
    
    // 短暂显示后移除
    auto fadeOut = FadeOut::create(0.3f);
    auto remove = RemoveSelf::create();
    attackEffect->runAction(Sequence::create(fadeOut, remove, nullptr));
}
