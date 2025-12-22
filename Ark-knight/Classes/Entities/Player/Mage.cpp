#include "Mage.h"
#include "Entities/Enemy/Enemy.h"

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
    fireball->setScale(0.5f);  // 调整火球大小
    
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
        float flyTime = 1.5f;
        float flyDistance = 600.0f;
        Vec2 startPos = fireball->getPosition();
        Vec2 endPos = startPos + direction * flyDistance;
        
        // 使用schedule来更新火球位置并检测碰撞
        fireball->schedule([fireball, parent, startPos, endPos, flyTime, fireballDamage](float dt) {
            // 检测与敌人的碰撞
            for (auto child : parent->getChildren())
            {
                if (child->getTag() == Constants::Tag::ENEMY)
                {
                    float dist = fireball->getPosition().distance(child->getPosition());
                    if (dist < 40.0f)  // 碰撞半径
                    {
                        // 造成伤害
                        auto enemy = dynamic_cast<Enemy*>(child);
                        if (enemy != nullptr && !enemy->isDead())
                        {
                            enemy->takeDamage(fireballDamage);
                            GAME_LOG("Fireball hits enemy for %d damage!", fireballDamage);
                            
                            // 火球命中后消失
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
