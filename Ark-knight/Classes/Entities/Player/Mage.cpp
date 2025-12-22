#include "Mage.h"
#include "Entities/Enemy/Enemy.h"
#include "Map/Room.h"

Mage::Mage()
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
    
    // 设置法师特有属性
    setMaxHP(80);           // 法师血量较低
    setHP(80);
    setMaxMP(150);          // 法师法力值高
    setMP(150);
    setAttack(15);          // 基础攻击力中等
    setMoveSpeed(180.0f);   // 移动速度中等
    
    GAME_LOG("Mage initialized - HP: %d/%d, MP: %d/%d", 
             getHP(), getMaxHP(), getMP(), getMaxMP());
    
    return true;
}

void Mage::attack()
{
    if (!canAttack())
    {
        return;
    }
    
    setState(EntityState::ATTACK);
    resetAttackCooldown();
    
    // 释放冰锥
    castIceShard();
    
    GAME_LOG("Mage casts Ice Shard!");
    
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

void Mage::useSkill()
{
    // 检查MP是否足够
    if (!consumeMP(getSkillMPCost()))
    {
        GAME_LOG("Not enough MP to cast Fireball!");
        return;
    }
    
    setState(EntityState::SKILL);
    
    // 释放火球
    castFireball();
    
    // 重置技能冷却
    resetSkillCooldown();
    
    GAME_LOG("Mage casts Fireball! MP: %d/%d", getMP(), getMaxMP());
    
    // 技能动画结束后返回IDLE
    auto delay = DelayTime::create(0.5f);
    auto callback = CallFunc::create([this]() {
        if (getState() == EntityState::SKILL)
        {
            setState(EntityState::IDLE);
        }
    });
    auto sequence = Sequence::create(delay, callback, nullptr);
    this->runAction(sequence);
}

void Mage::castFireball()
{
    GAME_LOG("Fireball cast in direction (%.2f, %.2f)", 
             _facingDirection.x, _facingDirection.y);
    
    // 创建火球精灵（使用动画）
    auto fireball = Sprite::create("UIs/Skills/Mage/FB500-1.png");
    fireball->setPosition(this->getPosition() + _facingDirection * 50);
    fireball->setTag(Constants::Tag::PROJECTILE);
    
    // 计算缩放比例：使火球大小接近地板分块(32px)
    float targetSize = Constants::FLOOR_TILE_SIZE * 1.2f;  // 略大于地板分块
    float scale = targetSize / fireball->getContentSize().width;
    fireball->setScale(scale);
    
    // 设置globalZOrder确保在正确层级显示
    fireball->setGlobalZOrder(Constants::ZOrder::PROJECTILE);
    
    // 创建火球动画（5帧循环）
    Vector<SpriteFrame*> fireballFrames;
    for (int i = 1; i <= 5; i++)
    {
        char filename[128];
        sprintf(filename, "UIs/Skills/Mage/FB500-%d.png", i);
        auto frame = Sprite::create(filename);
        if (frame)
        {
            fireballFrames.pushBack(frame->getSpriteFrame());
        }
    }
    
    if (!fireballFrames.empty())
    {
        auto animation = Animation::createWithSpriteFrames(fireballFrames, 0.08f);
        auto animate = Animate::create(animation);
        auto repeat = RepeatForever::create(animate);
        fireball->runAction(repeat);
    }
    
    // 保存伤害值和方向
    int fireballDamage = getAttack() * 2;  // 火球伤害是普攻的2倍
    Vec2 direction = _facingDirection;
    
    // 添加到父节点
    if (this->getParent() != nullptr)
    {
        auto parent = this->getParent();
        parent->addChild(fireball, Constants::ZOrder::PROJECTILE);
        
        // 火球飞行 + 碰撞检测
        float flyTime = 1.2f;
        float flyDistance = 600.0f;
        Vec2 startPos = fireball->getPosition();
        Vec2 endPos = startPos + direction * flyDistance;
        
        // 标记火球未命中
        fireball->setUserData(nullptr);
        
        // 使用schedule来更新火球位置并检测碰撞
        fireball->schedule([fireball, parent, fireballDamage](float dt) {
            if (!fireball->getParent() || fireball->getUserData() != nullptr) return;  // 已被移除或已命中
            
            Vec2 fireballPos = fireball->getPosition();
            float collisionRadius = Constants::FLOOR_TILE_SIZE * 0.5f;  // 碰撞半径
            
            // 检测是否碰到墙方块（遍历所有子节点，包括Room和Hallway）
            for (auto child : parent->getChildren())
            {
                // 检查节点内的墙
                for (auto subChild : child->getChildren())
                {
                    if (subChild->getTag() == Constants::Tag::WALL)
                    {
                        // 将墙的本地坐标转换为世界坐标
                        Vec2 wallWorldPos = child->convertToWorldSpace(subChild->getPosition());
                        // 再转换为parent的本地坐标系（与fireballPos同一坐标系）
                        Vec2 wallPos = parent->convertToNodeSpace(wallWorldPos);
                        float dist = fireballPos.distance(wallPos);
                        if (dist < collisionRadius)
                        {
                            GAME_LOG("Fireball hit wall!");
                            fireball->setUserData((void*)1);
                            fireball->stopAllActions();
                            fireball->unschedule("fireballUpdate");
                            fireball->removeFromParent();
                            return;
                        }
                    }
                }
            }
            
            // 检测与敌人的碰撞
            for (auto child : parent->getChildren())
            {
                if (child->getTag() == Constants::Tag::ENEMY)
                {
                    float dist = fireballPos.distance(child->getPosition());
                    if (dist < 40.0f)  // 碰撞半径
                    {
                        // 造成伤害
                        auto enemy = dynamic_cast<Enemy*>(child);
                        if (enemy != nullptr && !enemy->isDead())
                        {
                            enemy->takeDamage(fireballDamage);
                            GAME_LOG("Fireball hits enemy for %d damage!", fireballDamage);
                            
                            // 火球命中后消失
                            fireball->setUserData((void*)1);  // 标记已命中
                            fireball->stopAllActions();
                            fireball->unschedule("fireballUpdate");
                            fireball->removeFromParent();
                            return;
                        }
                    }
                }
            }
        }, "fireballUpdate");
        
        // 火球飞行动作
        auto moveTo = MoveTo::create(flyTime, endPos);
        auto remove = CallFunc::create([fireball]() {
            fireball->unschedule("fireballUpdate");
            fireball->removeFromParent();
        });
        auto sequence = Sequence::create(moveTo, remove, nullptr);
        fireball->runAction(sequence);
    }
}

void Mage::castIceShard()
{
    GAME_LOG("Ice Shard cast in direction (%.2f, %.2f)", 
             _facingDirection.x, _facingDirection.y);
    
    // 创建冰锥精灵（使用动画）
    auto iceShard = Sprite::create("UIs/Attacks/Mage/Ice_001.png");
    iceShard->setPosition(this->getPosition() + _facingDirection * 40);
    iceShard->setTag(Constants::Tag::PROJECTILE);
    
    // 计算缩放比例：使冰锥大小接近地板分块(32px)，增大25%
    float targetSize = Constants::FLOOR_TILE_SIZE * 1.25f;
    float scale = targetSize / iceShard->getContentSize().width;
    iceShard->setScale(scale);
    
    // 设置globalZOrder确保在正确层级显示
    iceShard->setGlobalZOrder(Constants::ZOrder::PROJECTILE);
    
    // 根据朝向旋转冰锥
    float angle = CC_RADIANS_TO_DEGREES(atan2(_facingDirection.y, _facingDirection.x));
    iceShard->setRotation(-angle);  // cocos2d旋转方向与数学方向相反
    
    // 创建冰锥形成动画（7帧，不循环）
    Vector<SpriteFrame*> iceFrames;
    for (int i = 1; i <= 7; i++)
    {
        char filename[128];
        sprintf(filename, "UIs/Attacks/Mage/Ice_%03d.png", i);
        auto frame = Sprite::create(filename);
        if (frame)
        {
            iceFrames.pushBack(frame->getSpriteFrame());
        }
    }
    
    if (!iceFrames.empty())
    {
        auto animation = Animation::createWithSpriteFrames(iceFrames, 0.06f);
        animation->setRestoreOriginalFrame(false);  // 保持最后一帧（完整冰锥）
        auto animate = Animate::create(animation);
        iceShard->runAction(animate);
    }
    
    // 保存伤害值和方向
    int iceDamage = getAttack();  // 冰锥伤害等于普攻
    Vec2 direction = _facingDirection;
    
    // 添加到父节点
    if (this->getParent() != nullptr)
    {
        auto parent = this->getParent();
        parent->addChild(iceShard, Constants::ZOrder::PROJECTILE);
        
        // 冰锥飞行 + 碰撞检测
        float flyTime = 1.0f;
        float flyDistance = 400.0f;
        Vec2 startPos = iceShard->getPosition();
        Vec2 endPos = startPos + direction * flyDistance;
        
        // 标记冰锥未命中
        iceShard->setUserData(nullptr);
        
        // 使用schedule来更新冰锥位置并检测碰撞
        iceShard->schedule([iceShard, parent, iceDamage](float dt) {
            if (!iceShard->getParent() || iceShard->getUserData() != nullptr) return;  // 已被移除或已命中
            
            Vec2 icePos = iceShard->getPosition();
            float collisionRadius = Constants::FLOOR_TILE_SIZE * 0.5f;  // 碰撞半径
            
            // 检测是否碰到墙方块（遍历所有子节点，包括Room和Hallway）
            for (auto child : parent->getChildren())
            {
                // 检查节点内的墙
                for (auto subChild : child->getChildren())
                {
                    if (subChild->getTag() == Constants::Tag::WALL)
                    {
                        // 将墙的本地坐标转换为世界坐标
                        Vec2 wallWorldPos = child->convertToWorldSpace(subChild->getPosition());
                        // 再转换为parent的本地坐标系（与icePos同一坐标系）
                        Vec2 wallPos = parent->convertToNodeSpace(wallWorldPos);
                        float dist = icePos.distance(wallPos);
                        if (dist < collisionRadius)
                        {
                            GAME_LOG("Ice Shard hit wall!");
                            iceShard->setUserData((void*)1);
                            iceShard->stopAllActions();
                            iceShard->unschedule("iceShardUpdate");
                            iceShard->removeFromParent();
                            return;
                        }
                    }
                }
            }
            
            // 检测与敌人的碰撞
            for (auto child : parent->getChildren())
            {
                if (child->getTag() == Constants::Tag::ENEMY)
                {
                    float dist = icePos.distance(child->getPosition());
                    if (dist < 35.0f)  // 碰撞半径
                    {
                        // 造成伤害
                        auto enemy = dynamic_cast<Enemy*>(child);
                        if (enemy != nullptr && !enemy->isDead())
                        {
                            enemy->takeDamage(iceDamage);
                            GAME_LOG("Ice Shard hits enemy for %d damage!", iceDamage);
                            
                            // 冰锥命中后消失
                            iceShard->setUserData((void*)1);  // 标记已命中
                            iceShard->stopAllActions();
                            iceShard->unschedule("iceShardUpdate");
                            iceShard->removeFromParent();
                            return;
                        }
                    }
                }
            }
        }, "iceShardUpdate");
        
        // 冰锥飞行动作
        auto moveTo = MoveTo::create(flyTime, endPos);
        auto remove = CallFunc::create([iceShard]() {
            iceShard->unschedule("iceShardUpdate");
            iceShard->removeFromParent();
        });
        auto sequence = Sequence::create(moveTo, remove, nullptr);
        iceShard->runAction(sequence);
    }
}
