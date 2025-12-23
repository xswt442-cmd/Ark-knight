#include "Mage.h"
#include "Entities/Enemy/Enemy.h"
#include "Map/Room.h"
#include "Map/Hallway.h"

Mage::Mage()
    : _isEnhanced(false)
    , _enhancedTimer(0.0f)
    , _enhancedDuration(8.0f)
    , _baseAttackInterval(1.0f)
    , _attackTimer(0.0f)
{
}

Mage::~Mage()
{
}

bool Mage::init()
{
    if (!Player::init())
    {
        return false;
    }
    
    // 设置妮芙属性
    setMaxHP(100);
    setHP(100);
    setMaxMP(150);
    setMP(150);
    setAttack(10);
    setMoveSpeed(180.0f);
    
    // 设置技能冷却
    _skillCooldown = 15.0f;
    
    // 设置攻击冷却为1秒（普通状态攻速）
    _attackCooldown = _baseAttackInterval;
    
    GAME_LOG("Nymph(Mage) initialized - HP: %d/%d, MP: %d/%d", 
             getHP(), getMaxHP(), getMP(), getMaxMP());
    
    return true;
}

void Mage::update(float dt)
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

void Mage::attack()
{
    if (!canAttack())
    {
        return;
    }
    
    setState(EntityState::ATTACK);
    resetAttackCooldown();
    
    // 发射子弹
    shootBullet();
    
    GAME_LOG("Nymph shoots bullet! Enhanced: %s", _isEnhanced ? "YES" : "NO");
    
    // 攻击动画结束后返回IDLE
    auto delay = DelayTime::create(0.3f);
    auto callback = CallFunc::create([this]() {
        if (_currentState == EntityState::ATTACK && !isDead())
        {
            setState(EntityState::IDLE);
        }
    });
    auto sequence = Sequence::create(delay, callback, nullptr);
    this->runAction(sequence);
}

void Mage::useSkill()
{
    // 检查MP是否足够
    if (!consumeMP(getSkillMPCost()))
    {
        GAME_LOG("Not enough MP to use skill!");
        return;
    }
    
    setState(EntityState::SKILL);
    
    // 进入强化状态
    enterEnhancedState();
    
    // 重置技能冷却
    resetSkillCooldown();
    
    GAME_LOG("Nymph enters enhanced state! Duration: %.1fs, MP: %d/%d", 
             _enhancedDuration, getMP(), getMaxMP());
    
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

void Mage::enterEnhancedState()
{
    _isEnhanced = true;
    _enhancedTimer = _enhancedDuration;
    
    // 强化状态：攻速提升100%（攻击间隔减半）
    _attackCooldown = _baseAttackInterval * 0.5f;
    
    GAME_LOG("Entered enhanced state - Attack interval: %.2fs", _attackCooldown);
}

void Mage::exitEnhancedState()
{
    _isEnhanced = false;
    _enhancedTimer = 0.0f;
    
    // 恢复正常攻速
    _attackCooldown = _baseAttackInterval;
    
    GAME_LOG("Exited enhanced state - Attack interval: %.2fs", _attackCooldown);
}

void Mage::shootBullet()
{
    GAME_LOG("Bullet shot in direction (%.2f, %.2f)", 
             _facingDirection.x, _facingDirection.y);
    
    // 根据强化状态选择子弹资源
    std::string bulletPath;
    if (_isEnhanced)
    {
        bulletPath = "Player/Nymph/Nymph_bullet/Nymph_Skill_Bullet1.png";
    }
    else
    {
        bulletPath = "Player/Nymph/Nymph_bullet/Nymph_Bullet1.png";
    }
    
    // 创建子弹精灵
    auto bullet = Sprite::create(bulletPath);
    if (!bullet)
    {
        GAME_LOG("Failed to create bullet sprite: %s", bulletPath.c_str());
        return;
    }
    
    bullet->setPosition(this->getPosition() + _facingDirection * 40);
    bullet->setTag(Constants::Tag::PROJECTILE);
    
    // 计算缩放比例
    float targetSize = Constants::FLOOR_TILE_SIZE * 1.0f;
    float scale = targetSize / bullet->getContentSize().width;
    bullet->setScale(scale);
    
    // 设置层级
    bullet->setGlobalZOrder(Constants::ZOrder::PROJECTILE);
    
    // 根据朝向旋转子弹
    float angle = CC_RADIANS_TO_DEGREES(atan2(_facingDirection.y, _facingDirection.x));
    bullet->setRotation(-angle);
    
    // 创建子弹动画（2帧循环）
    Vector<SpriteFrame*> bulletFrames;
    if (_isEnhanced)
    {
        for (int i = 1; i <= 2; i++)
        {
            char filename[128];
            sprintf(filename, "Player/Nymph/Nymph_bullet/Nymph_Skill_Bullet%d.png", i);
            auto frame = Sprite::create(filename);
            if (frame)
            {
                bulletFrames.pushBack(frame->getSpriteFrame());
            }
        }
    }
    else
    {
        for (int i = 1; i <= 2; i++)
        {
            char filename[128];
            sprintf(filename, "Player/Nymph/Nymph_bullet/Nymph_Bullet%d.png", i);
            auto frame = Sprite::create(filename);
            if (frame)
            {
                bulletFrames.pushBack(frame->getSpriteFrame());
            }
        }
    }
    
    if (!bulletFrames.empty())
    {
        auto animation = Animation::createWithSpriteFrames(bulletFrames, 0.1f);
        auto animate = Animate::create(animation);
        auto repeat = RepeatForever::create(animate);
        bullet->runAction(repeat);
    }
    
    // 计算伤害：普通100%攻击力，强化200%攻击力
    int bulletDamage = _isEnhanced ? getAttack() * 2 : getAttack();
    Vec2 direction = _facingDirection;
    
    // 添加到父节点
    if (this->getParent() != nullptr)
    {
        auto parent = this->getParent();
        parent->addChild(bullet, Constants::ZOrder::PROJECTILE);
        
        // 子弹飞行参数
        float flyTime = 1.0f;
        float flyDistance = 400.0f;
        Vec2 startPos = bullet->getPosition();
        Vec2 endPos = startPos + direction * flyDistance;
        
        // 使用UserData存储状态：0=正常, 1=已命中
        bullet->setUserData(nullptr);
        
        // 碰撞检测
        bullet->schedule([bullet, parent, bulletDamage](float dt) {
            if (!bullet->getParent() || bullet->getUserData() != nullptr) return;
            
            Vec2 bulletPos = bullet->getPosition();
            float wallCollisionRadius = Constants::FLOOR_TILE_SIZE * 1.0f;
            
            // 检测与墙的碰撞
            for (auto child : parent->getChildren())
            {
                for (auto subChild : child->getChildren())
                {
                    if (subChild->getTag() == Constants::Tag::WALL)
                    {
                        Vec2 wallWorldPos = child->convertToWorldSpace(subChild->getPosition());
                        Vec2 wallPos = parent->convertToNodeSpace(wallWorldPos);
                        
                        float dist = bulletPos.distance(wallPos);
                        if (dist < wallCollisionRadius)
                        {
                            bullet->setUserData((void*)1);
                            bullet->stopAllActions();
                            bullet->unschedule("bulletUpdate");
                            bullet->removeFromParent();
                            return;
                        }
                    }
                    
                    for (auto deepChild : subChild->getChildren())
                    {
                        if (deepChild->getTag() == Constants::Tag::WALL)
                        {
                            Vec2 wallWorldPos = subChild->convertToWorldSpace(deepChild->getPosition());
                            Vec2 wallPos = parent->convertToNodeSpace(wallWorldPos);
                            
                            float dist = bulletPos.distance(wallPos);
                            if (dist < wallCollisionRadius)
                            {
                                bullet->setUserData((void*)1);
                                bullet->stopAllActions();
                                bullet->unschedule("bulletUpdate");
                                bullet->removeFromParent();
                                return;
                            }
                        }
                    }
                }
            }
            
            // 检测与敌人的碰撞
            for (auto child : parent->getChildren())
            {
                if (child->getTag() == Constants::Tag::ENEMY)
                {
                    float dist = bulletPos.distance(child->getPosition());
                    if (dist < 35.0f)
                    {
                        auto enemy = dynamic_cast<Enemy*>(child);
                        if (enemy != nullptr && !enemy->isDead())
                        {
                            enemy->takeDamage(bulletDamage);
                            GAME_LOG("Bullet hits enemy for %d damage!", bulletDamage);
                            
                            bullet->setUserData((void*)1);
                            bullet->stopAllActions();
                            bullet->unschedule("bulletUpdate");
                            bullet->removeFromParent();
                            return;
                        }
                    }
                }
            }
        }, "bulletUpdate");
        
        // 子弹飞行动作
        auto moveTo = MoveTo::create(flyTime, endPos);
        auto remove = CallFunc::create([bullet]() {
            bullet->unschedule("bulletUpdate");
            bullet->removeFromParent();
        });
        auto sequence = Sequence::create(moveTo, remove, nullptr);
        bullet->runAction(sequence);
    }
}
