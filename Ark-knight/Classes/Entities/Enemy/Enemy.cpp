#include "Enemy.h"
#include "Entities/Player/Player.h"
#include "UI/FloatingText.h"
#include "Entities/Enemy/KongKaZi.h"
#include "Entities/Enemy/Cup.h"
#include "Scenes/GameScene.h"
#include "cocos2d.h"
#include <cmath>
#include <algorithm>

USING_NS_CC;

Enemy::Enemy()
    : _enemyType(EnemyType::MELEE)
    , _sightRange(Constants::Enemy::CHASE_RANGE)
    , _attackRange(Constants::Enemy::ATTACK_RANGE)
    , _patrolTarget(Vec2::ZERO)
    , _patrolTimer(0.0f)
    , _patrolInterval(2.0f)
    , _hasTarget(false)
    , _attackWindup(0.5f)
    , _poisonStacks(0)
    , _poisonTimer(0.0f)
    , _poisonTickAcc(0.0f)
    , _poisonSourceAttack(0)
    , _poisonColorSaved(false)
    , _stealthColorSaved(false)
    , _isRedMarked(false)
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

    setHP(Constants::Enemy::MELEE_HP);
    setMaxHP(Constants::Enemy::MELEE_HP);
    setMoveSpeed(Constants::Enemy::DEFAULT_MOVE_SPEED);
    setAttack(10);

    setTag(Constants::Tag::ENEMY);

    return true;
}

void Enemy::update(float dt)
{
    Character::update(dt);

    // 处理毒伤逻辑（保持原逻辑）
    if (_poisonStacks > 0)
    {
        _poisonTimer -= dt;
        _poisonTickAcc += dt;

        while (_poisonTickAcc >= POISON_TICK_INTERVAL)
        {
            _poisonTickAcc -= POISON_TICK_INTERVAL;

            if (_poisonSourceAttack > 0 && _poisonStacks > 0)
            {
                float dmgF = static_cast<float>(_poisonSourceAttack) * POISON_TICK_RATIO * static_cast<float>(_poisonStacks);
                int dmg = static_cast<int>(std::round(dmgF));
                if (dmg > 0)
                {
                    this->takeDamage(dmg);
                    Scene* running = Director::getInstance()->getRunningScene();
                    if (running)
                    {
                        Vec2 worldPos = this->convertToWorldSpace(Vec2::ZERO);
                        FloatingText::show(running, worldPos, std::to_string(dmg), Color3B(180,100,200));
                    }
                    GAME_LOG("Poison tick: %d damage applied to enemy (stacks=%d, srcAtk=%d)", dmg, _poisonStacks, _poisonSourceAttack);
                }
            }
        }

        if (_poisonTimer <= 0.0f)
        {
            _poisonStacks = 0;
            _poisonTimer = 0.0f;
            _poisonTickAcc = 0.0f;
            _poisonSourceAttack = 0;
            // 恢复颜色：如果当前处于隐身（存在 stealth 源），优先恢复为 stealth 原始色；否则恢复 poison 原始色
            if (_poisonColorSaved && _sprite)
            {
                if (!_stealthSources.empty() && _stealthColorSaved)
                {
                    // 仍处于隐身，保持隐身色（不覆盖）
                }
                else
                {
                    _sprite->setColor(_poisonOriginalColor);
                }
            }
            _poisonColorSaved = false;
            GAME_LOG("Poison expired on enemy, stacks cleared");
        }
    }
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

    // 根据当前是否处于隐身决定颜色：
    // - 非隐身：紫色（原来行为）
    // - 隐身中：使用更深的紫灰（以示同时中毒与隐身）
    if (_sprite)
    {
        if (_stealthSources.empty())
        {
            Color3B purple(180, 100, 200);
            _sprite->setColor(purple);
        }
        else
        {
            // 隐身且中毒：更深的紫灰色（可调整）
            Color3B deepStealthPoison(120, 80, 150);
            _sprite->setColor(deepStealthPoison);
        }
    }

    GAME_LOG("applyNymphPoison: stacks=%d, srcAtk=%d", _poisonStacks, _poisonSourceAttack);
}

// ==================== Stealth 源管理实现 ====================
void Enemy::addStealthSource(void* source)
{
    if (!source) return;
    // 如果已存在相同来源，忽略
    if (std::find(_stealthSources.begin(), _stealthSources.end(), source) != _stealthSources.end()) return;

    // 首次加入时保存当前颜色以便恢复
    if (_stealthSources.empty() && _sprite)
    {
        _stealthOriginalColor = _sprite->getColor();
        _stealthColorSaved = true;
    }

    _stealthSources.push_back(source);

    if (_sprite)
    {
        // 隐身时如果同时带毒，显示更深的毒色；否则显示浅灰隐身色
        if (_poisonStacks > 0)
        {
            Color3B deepStealthPoison(120, 80, 150);
            _sprite->setColor(deepStealthPoison);
        }
        else
        {
            _sprite->setColor(Color3B(180, 180, 180));
        }
    }
}

void Enemy::removeStealthSource(void* source)
{
    if (!source) return;
    auto it = std::find(_stealthSources.begin(), _stealthSources.end(), source);
    if (it == _stealthSources.end()) return;

    _stealthSources.erase(it);

    if (_stealthSources.empty())
    {
        // 没有隐身来源了，恢复颜色：若仍有毒则显示毒色，否则恢复之前保存的颜色
        if (_poisonStacks > 0 && _sprite)
        {
            Color3B purple(180, 100, 200);
            _sprite->setColor(purple);
        }
        else if (_stealthColorSaved && _sprite)
        {
            _sprite->setColor(_stealthOriginalColor);
            _stealthColorSaved = false;
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

// ==================== 红色标记与 KongKaZi 生成功能（不改变实体状态） ============
void Enemy::tryApplyRedMark(float chance)
{
    if (chance <= 0.0f) return;
    if (!canSpawnKongKaZiOnDeath()) return;
    if (_isRedMarked) return;

    float r = CCRANDOM_0_1();
    if (r <= chance)
    {
        _isRedMarked = true;
        if (_sprite)
        {
            _sprite->setColor(Color3B(255, 80, 80));
        }
        GAME_LOG("Enemy marked RED for KongKaZi spawn (chance succeeded)");
    }
}

void Enemy::die()
{
    // 仅处理“红色标记导致的紫圈 + 延迟生成 KongKaZi”视觉/生成逻辑
    // 不改变实体状态、不调用基类的 Character::die()，以免干扰子类自定义的死亡动画流程
    if (_isRedMarked && canSpawnKongKaZiOnDeath())
    {
        // 优先把紫色圆圈添加到敌人的父节点（通常为 gameLayer），使其位于地板之上、实体之下，行为类似 Cup 的范围圈
        Vec2 localPos = this->getPosition();
        Scene* running = Director::getInstance()->getRunningScene();

        auto circle = DrawNode::create();
        float radius = 50.0f;
        circle->drawSolidCircle(Vec2::ZERO, radius, 0, 32, Color4F(0.6f, 0.0f, 0.6f, 0.6f));

        Node* parent = this->getParent();
        if (parent)
        {
            int rangeZ = Constants::ZOrder::ENTITY - 1;
            parent->addChild(circle, rangeZ);
            circle->setPosition(localPos);
            circle->setGlobalZOrder(static_cast<float>(rangeZ));
        }
        else if (running)
        {
            // 退回到原始实现（运行场景坐标）
            Vec2 worldPos = this->convertToWorldSpace(Vec2::ZERO);
            running->addChild(circle, Constants::ZOrder::EFFECT);
            circle->setPosition(worldPos);
        }

        // 在圆自身上运行淡出与移除动作（避免对 running scene 调度）
        auto fade = FadeOut::create(0.5f);
        auto removeCircle = CallFunc::create([circle]() {
            if (circle->getParent()) circle->removeFromParent();
        });
        circle->runAction(Sequence::create(DelayTime::create(0.25f), fade, removeCircle, nullptr));

        // 延迟生成 KongKaZi（生成时使用本节点的本地坐标，相对通常为 gameLayer）
        Vec2 localSpawnPos = this->getPosition();
        auto spawnFunc = [localSpawnPos]() {
            auto kk = KongKaZi::create();
            if (!kk) return;

            kk->setPosition(localSpawnPos);
            kk->setTag(Constants::Tag::ENEMY);

            // 尝试通过当前运行场景注册到 GameScene，以便被 updateEnemies 管理
            auto runningInner = Director::getInstance()->getRunningScene();
            GameScene* gs = nullptr;
            if (runningInner)
            {
                gs = dynamic_cast<GameScene*>(runningInner);
                if (!gs)
                {
                    for (auto child : runningInner->getChildren())
                    {
                        gs = dynamic_cast<GameScene*>(child);
                        if (gs) break;
                    }
                }
            }

            if (gs)
            {
                gs->addEnemy(kk);
            }
            else
            {
                if (runningInner)
                {
                    runningInner->addChild(kk);
                }
            }
        };

        // 使用运行场景来调度延迟（若不存在则用 Director）
        if (running)
        {
            running->runAction(Sequence::create(DelayTime::create(0.28f), CallFunc::create(spawnFunc), nullptr));
        }
        else
        {
            Director::getInstance()->getRunningScene()->runAction(Sequence::create(DelayTime::create(0.28f), CallFunc::create(spawnFunc), nullptr));
        }
    }

    // 注意：不在这里调用 Character::die()，子类（例如 Ayao）会在自己的 die() 中执行完整的死亡动画与移除流程
}

/**
 * Enemy::takeDamage - 覆写以支持 Cup 的伤害分担逻辑
 */
void Enemy::takeDamage(int damage)
{
    // 复用GameEntity的基本过滤逻辑，但在分担场景下拆分伤害
    if (!_isAlive || damage <= 0)
    {
        return;
    }

    // 处于短暂无敌时忽略（与 GameEntity::takeDamage 一致）
    if (_hitInvulTimer > 0.0f)
    {
        return;
    }

    // 如果自身就是 Cup，则不再做分担检测，直接调用基类处理（遵循默认流程）
    Cup* selfCup = dynamic_cast<Cup*>(this);
    if (selfCup)
    {
        GameEntity::takeDamage(damage);
        return;
    }

    // 查找任意一个在范围内且未死亡的 Cup（以第一个为准）
    Cup* chosenCup = nullptr;
    const auto& cups = Cup::getInstances();
    for (auto c : cups)
    {
        if (!c) continue;
        if (c->isDead()) continue;
        float dist = c->getPosition().distance(this->getPosition());
        if (dist <= c->getShareRadius())
        {
            chosenCup = c;
            break;
        }
    }

    if (chosenCup)
    {
        // 将大部分伤害转交给 Cup（95%），受击者仅承受剩余（5%）
        float ratio = chosenCup->getShareRatio();
        int share = static_cast<int>(std::round(static_cast<float>(damage) * ratio));
        int remain = damage - share;
        if (remain < 0) remain = 0;

        // 先对受击者应用剩余伤害（保持 GameEntity 的受击显示与死亡逻辑）
        if (remain > 0)
        {
            GameEntity::takeDamage(remain);
        }
        else
        {
            // remain == 0 则我们仍然应该触发闪烁与 invul（保持最小的 visual feedback）
            // 这里我们通过调用 GameEntity::takeDamage(0) 无效果，因此手动设置短暂无敌并闪烁
            _hitInvulTimer = GameEntity::HIT_INVUL_DURATION;
            if (_sprite)
            {
                _sprite->stopActionByTag(100);
                auto blink = Blink::create(0.2f, 2);
                auto show = Show::create();
                auto seq = Sequence::create(blink, show, nullptr);
                seq->setTag(100);
                _sprite->runAction(seq);
            }
        }

        // 将分担的伤害给 Cup（Cup::absorbDamage 会直接减少 HP 并处理死亡）
        chosenCup->absorbDamage(share);

        GAME_LOG("Enemy::takeDamage redirected %d->Cup, remain %d to self (pos self=[%.1f,%.1f], cup=[%.1f,%.1f])",
                 share, remain,
                 this->getPosition().x, this->getPosition().y,
                 chosenCup->getPosition().x, chosenCup->getPosition().y);
    }
    else
    {
        // 未找到 Cup，默认行为
        GameEntity::takeDamage(damage);
    }
}

void Enemy::setRoomBounds(const cocos2d::Rect& bounds)
{
    // 默认空实现，子类可重写以接收房间边界
    (void)bounds;
}
