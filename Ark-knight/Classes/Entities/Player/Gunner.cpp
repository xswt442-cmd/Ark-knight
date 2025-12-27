#include "Gunner.h"
#include "Entities/Enemy/Enemy.h"
#include "Map/Room.h"
#include "Map/Hallway.h"
#include "UI/FloatingText.h"

Gunner::Gunner()
    : _isEnhanced(false)
    , _enhancedTimer(0.0f)
    , _enhancedDuration(8.0f)
    , _baseAttackInterval(1.0f)
    , _attackTimer(0.0f)
    , _currentAnimName("")
{
}

Gunner::~Gunner()
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

bool Gunner::init()
{
    if (!Player::init())
    {
        return false;
    }
    
    // 设置维什戴尔属性
    setMaxHP(100000);
    setHP(100000);
    setMaxMP(100);
    setMP(100);
    setAttack(500);
    setMoveSpeed(160.0f);
    
    // 设置技能冷却
    _skillCooldown = 15.0f;
    
    // 设置攻击冷却
    _attackCooldown = _baseAttackInterval;
    
    // 初始化动画
    initAnimations();
    
    // 创建精灵并绑定
    auto sprite = Sprite::create("Player/Wisdael/Wisdael_Idle/Wisdael_Idle_0001.png");
    if (sprite)
    {
        // 设置缩放（与法师一致）
        float targetSize = Constants::FLOOR_TILE_SIZE * 4.0f;
        float scale = targetSize / sprite->getContentSize().height;
        sprite->setScale(scale);
        
        bindSprite(sprite, Constants::ZOrder::ENTITY);
        
        // 播放默认待机动画
        playAnimation("idle", true);
    }
    
    GAME_LOG("Wisdael(Gunner) initialized - HP: %d/%d, MP: %d/%d", 
             getHP(), getMaxHP(), getMP(), getMaxMP());
    
    return true;
}

void Gunner::initAnimations()
{
    float frameDelay = 0.1f;
    
    // Idle动画 - 5帧
    {
        Vector<SpriteFrame*> frames;
        for (int i = 1; i <= 5; i++)
        {
            char filename[128];
            sprintf(filename, "Player/Wisdael/Wisdael_Idle/Wisdael_Idle_%04d.png", i);
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
    
    // Move动画 - 5帧
    {
        Vector<SpriteFrame*> frames;
        for (int i = 1; i <= 5; i++)
        {
            char filename[128];
            sprintf(filename, "Player/Wisdael/Wisdael_Move/Wisdael_Move_%04d.png", i);
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
            sprintf(filename, "Player/Wisdael/Wisdael_Attack/Wisdael_Attack_%04d.png", i);
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
            _animations["attack"] = anim;
        }
    }
    
    // Die动画 - 7帧
    {
        Vector<SpriteFrame*> frames;
        for (int i = 1; i <= 7; i++)
        {
            char filename[128];
            sprintf(filename, "Player/Wisdael/Wisdael_Die/Wisdael_Die_%04d.png", i);
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
    
    // Skill_Idle动画 - 5帧
    {
        Vector<SpriteFrame*> frames;
        for (int i = 1; i <= 5; i++)
        {
            char filename[128];
            sprintf(filename, "Player/Wisdael/Wisdael_Skill_Idle/Wisdael_Skill_Idle_%04d.png", i);
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
    
    // Skill_Move动画 - 5帧
    {
        Vector<SpriteFrame*> frames;
        for (int i = 1; i <= 5; i++)
        {
            char filename[128];
            sprintf(filename, "Player/Wisdael/Wisdael_Skill_Move/Wisdael_Skill_Move_%04d.png", i);
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
    
    // Skill_Attack动画 - 7帧
    {
        Vector<SpriteFrame*> frames;
        for (int i = 1; i <= 7; i++)
        {
            char filename[128];
            sprintf(filename, "Player/Wisdael/Wisdael_Skill_Attack/Wisdael_Skill_Attack_%04d.png", i);
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
            _animations["skill_attack"] = anim;
        }
    }
    
    GAME_LOG("Wisdael animations loaded: %zu animations", _animations.size());
}

void Gunner::playAnimation(const std::string& name, bool loop)
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

void Gunner::setState(EntityState state)
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

void Gunner::update(float dt)
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

void Gunner::attack()
{
    if (!canAttack())
    {
        return;
    }
    
    setState(EntityState::ATTACK);
    resetAttackCooldown();
    
    // 发射子弹
    shootBullet();
    
    GAME_LOG("Wisdael shoots bullet! Enhanced: %s", _isEnhanced ? "YES" : "NO");
    
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

void Gunner::useSkill()
{
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
    
    GAME_LOG("Wisdael enters enhanced state! Duration: %.1fs, MP: %d/%d", 
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

void Gunner::enterEnhancedState()
{
    _isEnhanced = true;
    _enhancedTimer = _enhancedDuration;
    
    // 强化状态：攻速提升
    _attackCooldown = _baseAttackInterval * 0.5f;
    
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

void Gunner::exitEnhancedState()
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

void Gunner::shootBullet()
{
    // 根据强化状态选择子弹资源和伤害
    std::string bulletPath = "Player/Wisdael/Wisdael_bullet/Wisdael_bullet_0001.png";
    
    // 创建子弹精灵
    auto bullet = Sprite::create(bulletPath);
    if (!bullet)
    {
        GAME_LOG("Failed to create bullet sprite: %s", bulletPath.c_str());
        return;
    }
    
    bullet->setPosition(this->getPosition() + _facingDirection * 40);
    bullet->setTag(Constants::Tag::PROJECTILE);
    
    // 设置缩放
    float targetSize = Constants::FLOOR_TILE_SIZE * 2.0f;
    float scale = targetSize / bullet->getContentSize().width;
    bullet->setScale(scale);
    
    bullet->setGlobalZOrder(Constants::ZOrder::PROJECTILE);
    
    // 根据朝向旋转子弹
    float angle = CC_RADIANS_TO_DEGREES(atan2(_facingDirection.y, _facingDirection.x));
    bullet->setRotation(-angle);
    
    // 创建子弹动画（3帧循环）
    Vector<SpriteFrame*> bulletFrames;
    for (int i = 1; i <= 3; i++)
    {
        char filename[128];
        sprintf(filename, "Player/Wisdael/Wisdael_bullet/Wisdael_bullet_%04d.png", i);
        auto frame = Sprite::create(filename);
        if (frame)
        {
            bulletFrames.pushBack(frame->getSpriteFrame());
        }
    }
    
    if (!bulletFrames.empty())
    {
        auto animation = Animation::createWithSpriteFrames(bulletFrames, 0.1f);
        auto animate = Animate::create(animation);
        auto repeat = RepeatForever::create(animate);
        bullet->runAction(repeat);
    }
    
    // 计算伤害：普通100%攻击力，强化300%攻击力
    int bulletDamage = _isEnhanced ? getAttack() * 3 : getAttack();
    Vec2 direction = _facingDirection;
    bool isEnhanced = _isEnhanced;
    float explosionRadius = _isEnhanced ? 100.0f : 50.0f;  // 强化时爆炸范围更大
    
    // 添加到父节点
    if (this->getParent() != nullptr)
    {
        auto parent = this->getParent();
        parent->addChild(bullet, Constants::ZOrder::PROJECTILE);
        
        float flyTime = 0.8f;
        float flyDistance = 350.0f;
        Vec2 startPos = bullet->getPosition();
        Vec2 endPos = startPos + direction * flyDistance;
        
        bullet->setUserData(nullptr);
        
        // 碰撞检测
        bullet->schedule([bullet, parent, bulletDamage, explosionRadius, isEnhanced, this](float dt) {
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
                            // 爆炸范围伤害
                            this->createExplosion(parent, bulletPos, bulletDamage, explosionRadius);
                            
                            bullet->setUserData((void*)1);
                            bullet->stopAllActions();
                            bullet->unschedule("bulletUpdate");
                            bullet->removeFromParent();
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
                    float dist = bulletPos.distance(child->getPosition());
                    if (dist < 35.0f)
                    {
                        auto enemy = dynamic_cast<Enemy*>(child);
                        if (enemy != nullptr && !enemy->isDead())
                        {
                            if (enemy->isStealthed()) continue;
                            
                            // 爆炸范围伤害
                            this->createExplosion(parent, bulletPos, bulletDamage, explosionRadius);
                            
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
        auto explode = CallFunc::create([bullet, parent, bulletDamage, explosionRadius, this]() {
            if (bullet->getUserData() == nullptr)
            {
                // 到达终点爆炸
                this->createExplosion(parent, bullet->getPosition(), bulletDamage, explosionRadius);
            }
            bullet->unschedule("bulletUpdate");
            bullet->removeFromParent();
        });
        auto sequence = Sequence::create(moveTo, explode, nullptr);
        bullet->runAction(sequence);
    }
}

void Gunner::shootSkillBomb()
{
    // 技能炸弹使用更大的范围和更高的伤害
    shootBullet();  // 简化实现，复用子弹逻辑
}

// 创建爆炸效果并造成范围伤害
void Gunner::createExplosion(Node* parent, const Vec2& pos, int damage, float radius)
{
    // 创建爆炸视觉效果
    Vector<SpriteFrame*> boomFrames;
    for (int i = 1; i <= 5; i++)
    {
        char filename[128];
        sprintf(filename, "Player/Wisdael/Wisdael_bullet/Wisdael_Boom_%04d.png", i);
        auto frame = Sprite::create(filename);
        if (frame)
        {
            boomFrames.pushBack(frame->getSpriteFrame());
        }
    }
    
    if (!boomFrames.empty())
    {
        auto explosion = Sprite::createWithSpriteFrame(boomFrames.front());
        explosion->setPosition(pos);
        explosion->setScale(radius / 50.0f);  // 根据爆炸半径调整大小
        explosion->setGlobalZOrder(Constants::ZOrder::PROJECTILE + 1);
        parent->addChild(explosion);
        
        auto animation = Animation::createWithSpriteFrames(boomFrames, 0.08f);
        auto animate = Animate::create(animation);
        auto remove = RemoveSelf::create();
        explosion->runAction(Sequence::create(animate, remove, nullptr));
    }
    
    // 范围伤害 - 先收集敌人避免遍历时修改容器导致迭代器失效
    std::vector<Enemy*> enemiesToHit;
    for (auto child : parent->getChildren())
    {
        if (child->getTag() == Constants::Tag::ENEMY)
        {
            float dist = pos.distance(child->getPosition());
            if (dist < radius)
            {
                auto enemy = dynamic_cast<Enemy*>(child);
                if (enemy != nullptr && !enemy->isDead() && !enemy->isStealthed())
                {
                    enemiesToHit.push_back(enemy);
                }
            }
        }
    }
    
    // 对收集到的敌人造成伤害
    for (auto enemy : enemiesToHit)
    {
        if (enemy && !enemy->isDead())
        {
            int oldHP = enemy->getHP();
            enemy->takeDamage(damage);
            int applied = oldHP - enemy->getHP();
            if (applied > 0)
            {
                FloatingText::show(parent, enemy->getPosition(), std::to_string(applied), Color3B(255, 100, 0));
            }
            GAME_LOG("Explosion hits enemy for %d damage!", applied);
        }
    }
}
