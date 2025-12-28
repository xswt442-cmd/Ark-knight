#include "Warrior.h"
#include "cocos2d.h"
#include "Entities/Enemy/Enemy.h"
#include "Map/Room.h"
#include "Map/Hallway.h"
#include "UI/FloatingText.h"
#include "audio/include/AudioEngine.h"
#include "Managers/SoundManager.h"

USING_NS_CC;

Warrior::Warrior()
    : _isEnhanced(false)
    , _enhancedTimer(0.0f)
    , _enhancedDuration(20.0f)
    , _baseAttackInterval(0.4f)
    , _baseAttackRange(120.0f) // 增大基础攻击范围 (原80.0f)
    , _attackTimer(0.0f)
    , _currentAnimName("")
    , _currentShield(0)
    , _shieldBarNode(nullptr)
    , _shieldLabel(nullptr)
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
    
    // 预加载音效
    SoundManager::getInstance()->preload("SoundEffect/MudRock_Attack.mp3");
    SoundManager::getInstance()->preload("SoundEffect/MudRock_Skill_Attack.mp3");
    
    // 设置泥岩属性（高血量，低蓝）
    setMaxHP(150000);
    setHP(150000);
    setMaxMP(200);
    setMP(200);
    setAttack(800);
    setMoveSpeed(180.0f);  // 战士移速较慢
    
    // 设置技能冷却
    _skillCooldown = 18.0f;
    
    // 设置攻击冷却
    _attackCooldown = _baseAttackInterval;
    
    // 初始化动画
    initAnimations();
    
    // 创建精灵并绑定
    auto sprite = Sprite::create("Player/Mudrock/MudRock_Idle/MudRock_Idle_0001.png");
    float targetSize = Constants::FLOOR_TILE_SIZE * 4.5f;
    
    if (sprite)
    {
        // 设置缩放（战士稍微大一点）
        float scale = targetSize / sprite->getContentSize().height;
        sprite->setScale(scale);
        
        bindSprite(sprite, Constants::ZOrder::ENTITY);
        
        // 播放默认待机动画
        playAnimation("idle", true);
    }
    
    // 初始化护盾条与数值文本，作为角色子节点（避免每帧 reparent 导致崩溃）
    _shieldBarNode = DrawNode::create();
    _shieldLabel = Label::createWithSystemFont("", "Arial", 14);
    _shieldLabel->setAnchorPoint(Vec2(0.5f, 1.0f)); // 文本位于护盾条下方
    // 将护盾条和文本作为角色子节点，使用极大 globalZOrder 确保最上层显示
    this->addChild(_shieldBarNode);
    this->addChild(_shieldLabel);
    _shieldBarNode->setGlobalZOrder(1000000);
    _shieldLabel->setGlobalZOrder(1000000);
    // 初始位置（局部坐标），距离精灵更近
    float initialOffsetY = Constants::FLOOR_TILE_SIZE * 4.5f * 0.55f;
    _shieldBarNode->setPosition(Vec2(0, initialOffsetY));
    _shieldLabel->setPosition(Vec2(0, initialOffsetY - 8.0f - 4.0f));
    // 先清空显示
    updateShieldBar();

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
    
    // ---- 替换 update() 中原来 reparent 的大块，改为只更新本地位置 ----
    // 每帧同步护盾条位置（仅更新位置，不做 reparent）
    if (_shieldBarNode != nullptr && _shieldLabel != nullptr)
    {
        // 护盾条局部偏移（相对于 Warrior）
        float offsetY = Constants::FLOOR_TILE_SIZE * 4.5f * 0.55f; // 可根据需要微调
        Vec2 localBarPos = Vec2(0, offsetY);
        // 直接设置为子节点的本地位置（this 是它们父节点）
        _shieldBarNode->setPosition(localBarPos);
        // 文本位于护盾条下方
        float barHeight = 8.0f;
        _shieldLabel->setPosition(localBarPos + Vec2(0, -barHeight/2 - 6.0f));
        // 保证 global ZOrder（若 init 未设置或被其它代码改动过，重新确保）
        _shieldBarNode->setGlobalZOrder(1000000);
        _shieldLabel->setGlobalZOrder(1000000);
    }
    // ---- update() 替换结束 ----
}

void Warrior::attack()
{
    if (!canAttack())
    {
        return;
    }
    
    setState(EntityState::ATTACK);
    resetAttackCooldown();
    
    // 使用 SoundManager 播放攻击音效
    if (_isEnhanced)
    {
        SoundManager::getInstance()->playSFX("SoundEffect/MudRock_Skill_Attack.mp3");
    }
    else
    {
        SoundManager::getInstance()->playSFX("SoundEffect/MudRock_Attack.mp3");
    }
    
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
    float attackAngle = 160.0f;  // 增大攻击扇形角度 (原120.0f)
    
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
    
    // 1. 增加护盾逻辑 (仅当攻击命中敌人时)
    if (!enemiesToHit.empty())
    {
        // 普通状态：2%最大生命值，强化状态：5%最大生命值
        float shieldRatio = _isEnhanced ? 0.05f : 0.02f;
        int shieldAmount = static_cast<int>(getMaxHP() * shieldRatio);
        addShield(shieldAmount);
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

void Warrior::addShield(int amount)
{
    _currentShield += amount;
    
    // 上限不超过最大生命3倍
    int maxShield = getMaxHP() * 3;
    if (_currentShield > maxShield)
    {
        _currentShield = maxShield;
    }
    
    updateShieldBar();
}

void Warrior::takeDamage(int damage)
{
    // 优先扣除护盾
    if (_currentShield > 0)
    {
        int absorbed = std::min(damage, _currentShield);
        _currentShield -= absorbed;
        damage -= absorbed;
        
        updateShieldBar();
        
        // 显示护盾吸收效果（可选）
        if (absorbed > 0 && this->getParent())
        {
            FloatingText::show(this->getParent(), this->getPosition() + Vec2(0, 120), 
                              "Absorb", Color3B(200, 200, 200));
        }
    }
    
    // 剩余伤害扣血
    if (damage > 0)
    {
        Player::takeDamage(damage);
    }
}

// ---- 微调 updateShieldBar()：只绘制与文本赋值（确保无父变更） ----
void Warrior::updateShieldBar()
{
    if (!_shieldBarNode || !_shieldLabel) return;
    _shieldBarNode->clear();

    if (_currentShield <= 0)
    {
        _shieldLabel->setString("");
        return;
    }

    // 绘制护盾条
    float width = 100.0f;
    float height = 8.0f;

    // 背景（更深的灰）
    _shieldBarNode->drawSolidRect(Vec2(-width/2, -height/2), Vec2(width/2, height/2), Color4F(0.15f, 0.15f, 0.15f, 0.9f));

    // 护盾值（灰色，深一点）
    float maxDisplayShield = getMaxHP() * 1.0f;
    float percent = (float)_currentShield / maxDisplayShield;
    if (percent > 1.0f) percent = 1.0f;
    float fillWidth = width * percent;
    _shieldBarNode->drawSolidRect(Vec2(-width/2, -height/2), Vec2(-width/2 + fillWidth, height/2), Color4F(0.55f, 0.55f, 0.55f, 1.0f));

    // 显示护盾数值文本（在护盾条下方）
    char buf[64];
    sprintf(buf, "%d", _currentShield);
    _shieldLabel->setString(buf);
    _shieldLabel->setTextColor(Color4B(230, 230, 230, 255));
    _shieldLabel->setSystemFontSize(14);
}
// ---- updateShieldBar() 替换结束 ----
