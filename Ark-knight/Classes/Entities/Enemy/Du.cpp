#include "Du.h"
#include "cocos2d.h"
#include "Entities/Player/Player.h"
#include "Scenes/GameScene.h"
#include "UI/FloatingText.h"
#include "Core/Constants.h"
#include <algorithm>

USING_NS_CC;

static const int DU_MOVE_ACTION_TAG = 0xD201;
static const int DU_WINDUP_ACTION_TAG = 0xD202;
// 原来的固定飞行时间保留为回退值
static constexpr float DU_BULLET_FLIGHT_TIME = 3.0f;
// 新增：以像素/秒为单位的子弹速度（调整此值可直接改变子弹速度）
static constexpr float DU_BULLET_SPEED = 700.0f;
static const char* DU_BULLET_SCHEDULE_KEY = "DuBulletUpdate";

Du::Du()
    : _moveAnimation(nullptr)
    , _attackAnimation(nullptr)
    , _dieAnimation(nullptr)
    , _bulletAnimation(nullptr)
    , _hasRoomBounds(false)
    , _isFiring(false)
    , _currentBullet(nullptr)
    , _attackTarget(nullptr) // 新增：初始化攻击目标指针
{
}

Du::~Du()
{
    CC_SAFE_RELEASE(_moveAnimation);
    CC_SAFE_RELEASE(_attackAnimation);
    CC_SAFE_RELEASE(_dieAnimation);
    CC_SAFE_RELEASE(_bulletAnimation);
}

bool Du::init()
{
    if (!Enemy::init()) return false;

    setEnemyType(EnemyType::RANGED);

    setupAttributes();
    loadAnimations();

    // 使用移动动画第一帧作为初始精灵（与 Ayao/TangHuang 风格一致）
    if (_moveAnimation && !_moveAnimation->getFrames().empty())
    {
        auto sprite = Sprite::createWithSpriteFrame(_moveAnimation->getFrames().front()->getSpriteFrame());
        // 放大 1.5 倍
        sprite->setScale(1.3f);
        this->bindSprite(sprite);
    }

    GAME_LOG("Du initialized");
    return true;
}

void Du::setupAttributes()
{
    // 依据需求：远程，攻击前摇 0.8s，索敌范围大，伤害偏高
    setMaxHP(3500);
    setHP(3500);

    setAttack(1800); // 造成大量伤害（可根据平衡调整）
    setMoveSpeed(65.0f);

    setSightRange(500.0f);   // 远程索敌大范围
    setAttackRange(420.0f);  // 远程攻击范围
    setAttackCooldown(2.5f);
    setAttackWindup(0.8f);
}

void Du::loadAnimations()
{
    // Move
    Vector<SpriteFrame*> moveFrames;
    for (int i = 1; i <= 6; ++i)
    {
        char filename[256];
        sprintf(filename, "Enemy/Du/Du_Move/Du_Move_%04d.png", i);
        SpriteFrame* frame = nullptr;
        auto s = Sprite::create(filename);
        if (s) frame = s->getSpriteFrame();

        if (!frame)
        {
            char basename[128];
            sprintf(basename, "Du_Move_%04d.png", i);
            frame = SpriteFrameCache::getInstance()->getSpriteFrameByName(basename);
        }

        if (frame) moveFrames.pushBack(frame);
        else GAME_LOG("Du: failed to load move frame: %s", filename);
    }
    if (!moveFrames.empty())
    {
        _moveAnimation = Animation::createWithSpriteFrames(moveFrames, 0.12f);
        _moveAnimation->retain();
    }

    // Attack (前摇/发射动画)
    Vector<SpriteFrame*> attackFrames;
    for (int i = 1; i <= 6; ++i)
    {
        char filename[256];
        sprintf(filename, "Enemy/Du/Du_Attack/Du_Attack_%04d.png", i);
        SpriteFrame* frame = nullptr;
        auto s = Sprite::create(filename);
        if (s) frame = s->getSpriteFrame();

        if (!frame)
        {
            char basename[128];
            sprintf(basename, "Du_Attack_%04d.png", i);
            frame = SpriteFrameCache::getInstance()->getSpriteFrameByName(basename);
        }

        if (frame) attackFrames.pushBack(frame);
        else GAME_LOG("Du: failed to load attack frame: %s", filename);
    }
    if (!attackFrames.empty())
    {
        _attackAnimation = Animation::createWithSpriteFrames(attackFrames, 0.10f);
        _attackAnimation->retain();
    }

    // Bullet 飞行动画
    Vector<SpriteFrame*> bulletFrames;
    for (int i = 1; i <= 6; ++i)
    {
        char filename[256];
        sprintf(filename, "Enemy/Du/Du_Bullet/Du_Bullet_%04d.png", i);
        SpriteFrame* frame = nullptr;
        auto s = Sprite::create(filename);
        if (s) frame = s->getSpriteFrame();

        if (!frame)
        {
            char basename[128];
            sprintf(basename, "Du_Bullet_%04d.png", i);
            frame = SpriteFrameCache::getInstance()->getSpriteFrameByName(basename);
        }

        if (frame) bulletFrames.pushBack(frame);
        else GAME_LOG("Du: failed to load bullet frame: %s", filename);
    }
    if (!bulletFrames.empty())
    {
        _bulletAnimation = Animation::createWithSpriteFrames(bulletFrames, 0.08f);
        _bulletAnimation->retain();
    }

    // Die
    Vector<SpriteFrame*> dieFrames;
    for (int i = 1; i <= 5; ++i)
    {
        char filename[256];
        sprintf(filename, "Enemy/Du/Du_Die/Du_Die_%04d.png", i);
        SpriteFrame* frame = nullptr;
        auto s = Sprite::create(filename);
        if (s) frame = s->getSpriteFrame();

        if (!frame)
        {
            char basename[128];
            sprintf(basename, "Du_Die_%04d.png", i);
            frame = SpriteFrameCache::getInstance()->getSpriteFrameByName(basename);
        }

        if (frame) dieFrames.pushBack(frame);
        else GAME_LOG("Du: failed to load die frame: %s", filename);
    }
    if (!dieFrames.empty())
    {
        _dieAnimation = Animation::createWithSpriteFrames(dieFrames, 0.12f);
        _dieAnimation->retain();
    }

    GAME_LOG("Du animations loaded");
}

void Du::update(float dt)
{
    if (_currentState == EntityState::DIE) return;

    Enemy::update(dt);

    // 限制房间边界
    if (_hasRoomBounds)
    {
        Vec2 pos = this->getPosition();
        pos.x = std::max(_roomBounds.getMinX(), std::min(pos.x, _roomBounds.getMaxX()));
        pos.y = std::max(_roomBounds.getMinY(), std::min(pos.y, _roomBounds.getMaxY()));
        this->setPosition(pos);
    }

    // 如果处于发射等待中则停止一切移动行为（外部 move 不应生效）
    if (_isFiring)
    {
        // 确保移动动画停止
        if (_sprite)
        {
            auto moveAct = _sprite->getActionByTag(DU_MOVE_ACTION_TAG);
            if (moveAct) _sprite->stopAction(moveAct);
        }
    }
}

void Du::executeAI(Player* player, float dt)
{
    if (!player || player->isDead() || _currentState == EntityState::DIE) return;

    // 若正在发射则不做其他AI（保持停住直到子弹解决）
    if (_isFiring) return;

    if (isPlayerInSight(player))
    {
        _hasTarget = true;

        if (isPlayerInAttackRange(player))
        {
            _attackTarget = player;
            attack();
        }
        else
        {
            // 追击玩家（只有不在发射期间才会追）
            chasePlayer(player, dt);
        }
    }
    else
    {
        _hasTarget = false;
        _attackTarget = nullptr;
        patrol(dt);
    }
}

void Du::attack()
{
    if (!canAttack() || _currentState == EntityState::DIE) return;

    setState(EntityState::ATTACK);
    resetAttackCooldown();

    // 停止移动循环动作（仅停止移动 action）
    if (_sprite)
    {
        auto moveAct = _sprite->getActionByTag(DU_MOVE_ACTION_TAG);
        if (moveAct) _sprite->stopAction(moveAct);
    }

    // 播放前摇（若有 attack 动画）
    if (_attackAnimation && _sprite)
    {
        // 如果之前有前摇，先停止，保持行为一致
        auto prevWind = _sprite->getActionByTag(DU_WINDUP_ACTION_TAG);
        if (prevWind) _sprite->stopAction(prevWind);

        _attackAnimation->setRestoreOriginalFrame(true);
        auto animate = Animate::create(_attackAnimation);
        animate->setTag(DU_WINDUP_ACTION_TAG);
        _sprite->runAction(animate);

        float windup = this->getAttackWindup();
        Player* target = _attackTarget;
        auto delay = DelayTime::create(windup);
        // 在 windup 完成时实际发射子弹（并进入发射等待状态直到子弹结束）
        auto callback = CallFunc::create([this, target]() {
            if (_currentState == EntityState::ATTACK)
            {
                // 面向目标并发射
                if (target && !target->isDead())
                {
                    this->faceToPosition(target->getPosition());
                    this->fireBullet(target);
                }
                else
                {
                    // 目标丢失则恢复为空闲
                    if (this->_currentState == EntityState::ATTACK) setState(EntityState::IDLE);
                }
            }
        });
        auto seq = Sequence::create(delay, callback, nullptr);
        this->runAction(seq);
    }
    else
    {
        // 无动画时使用 windup 定时并发射
        float windup = this->getAttackWindup();
        Player* target = _attackTarget;
        auto delay = DelayTime::create(windup);
        auto callback = CallFunc::create([this, target]() {
            if (_currentState == EntityState::ATTACK)
            {
                if (target && !target->isDead())
                {
                    this->faceToPosition(target->getPosition());
                    this->fireBullet(target);
                }
                else
                {
                    if (this->_currentState == EntityState::ATTACK) setState(EntityState::IDLE);
                }
            }
        });
        this->runAction(Sequence::create(delay, callback, nullptr));
    }

    GAME_LOG("Du begins attack (windup)!");
}

void Du::fireBullet(Player* target)
{
    if (!target)
    {
        // 恢复状态
        setState(EntityState::IDLE);
        return;
    }

    // 标记正在发射（期间不移动）
    _isFiring = true;

    // 生成子弹 sprite（优先图片文件，再 fallback SpriteFrameCache）
    Sprite* bulletSprite = nullptr;
    auto s = Sprite::create("Enemy/Du/Du_Bullet/Du_Bullet_0001.png");
    if (s) bulletSprite = s;
    else
    {
        auto frame = SpriteFrameCache::getInstance()->getSpriteFrameByName("Du_Bullet_0001.png");
        if (frame) bulletSprite = Sprite::createWithSpriteFrame(frame);
    }

    Node* bulletNode = nullptr;
    if (bulletSprite)
    {
        bulletNode = bulletSprite;
    }
    else
    {
        // 兜底：创建一个小圆形 DrawNode 表示子弹（不应发生）
        auto dn = DrawNode::create();
        dn->drawSolidCircle(Vec2::ZERO, 6.0f, 0, 16, Color4F(1,0,0,1));
        bulletNode = dn;
    }

    // 添加到与角色相同的父节点（通常为 gameLayer）
    Node* parent = this->getParent();
    if (parent)
    {
        int z = Constants::ZOrder::PROJECTILE;
        parent->addChild(bulletNode, z);
        bulletNode->setPosition(this->getPosition());
    }
    else
    {
        this->addChild(bulletNode, 1);
        bulletNode->setPosition(Vec2::ZERO);
    }

    _currentBullet = bulletNode;

    // 播放子弹飞行动画（循环），仅当是 Sprite 时
    if (_bulletAnimation && bulletSprite)
    {
        auto animate = Animate::create(_bulletAnimation);
        auto loop = RepeatForever::create(animate);
        bulletSprite->runAction(loop);
    }

    // 目标当前位置（发射时锁定）
    Vec2 targetPos = target->getPosition();
    Vec2 startPos = bulletNode->getPosition();
    Vec2 dir = targetPos - startPos;
    if (dir.lengthSquared() > 0.0001f)
    {
        float angle = CC_RADIANS_TO_DEGREES(atan2(dir.y, dir.x));
        // Cocos 的 setRotation 以逆时针为正，调整符号以使朝向正确
        bulletNode->setRotation(-angle);
    }

    // 设置层级（与其他投射物一致）
    if (auto spr = dynamic_cast<Sprite*>(bulletNode))
    {
        spr->setGlobalZOrder(Constants::ZOrder::PROJECTILE);
    }

    // 启动每帧检测，处理命中玩家 / 碰撞边界 / 障碍（改为更稳健的世界坐标距离检测，避免穿墙或无法命中）
    bulletNode->schedule([this, bulletNode, targetPos](float dt) {
        // 若 bullet 已移除，中止
        if (!bulletNode->getParent()) return;

        // 获取运行场景与 GameScene、玩家引用
        Scene* running = Director::getInstance()->getRunningScene();
        GameScene* gs = nullptr;
        if (running)
        {
            gs = dynamic_cast<GameScene*>(running);
            if (!gs)
            {
                for (auto child : running->getChildren())
                {
                    gs = dynamic_cast<GameScene*>(child);
                    if (gs) break;
                }
            }
        }
        Player* player = gs ? gs->getPlayer() : nullptr;

        // 计算子弹世界坐标
        Vec2 bulletWorldPos;
        if (bulletNode->getParent())
            bulletWorldPos = bulletNode->getParent()->convertToWorldSpace(bulletNode->getPosition());
        else
            bulletWorldPos = bulletNode->getPosition();

        // 1) 检测与玩家的碰撞（使用世界坐标半径检测，兼容不同父节点）
        if (player && !player->isDead())
        {
            Vec2 playerWorldPos;
            if (player->getParent())
                playerWorldPos = player->getParent()->convertToWorldSpace(player->getPosition());
            else
                playerWorldPos = player->getPosition();

            // 以两个包围盒的半径作为碰撞半径
            float bulletRadius = 10.0f;
            Rect bb = bulletNode->getBoundingBox();
            if (bb.size.width > 0.0f) bulletRadius = std::max(6.0f, std::min(bb.size.width, bb.size.height) * 0.5f);

            float playerRadius = 20.0f;
            if (player->getSprite())
            {
                Rect pb = player->getSprite()->getBoundingBox();
                playerRadius = std::max(12.0f, std::min(pb.size.width, pb.size.height) * 0.25f);
            }

            float distToPlayer = bulletWorldPos.distance(playerWorldPos);
            if (distToPlayer <= (bulletRadius + playerRadius))
            {
                // 命中造成大量伤害（使用 Du 的攻击力）
                int dmg = this->getAttack();
                player->takeDamage(dmg);
                FloatingText::show(bulletNode->getParent(), player->getPosition(), std::to_string(dmg), Color3B(220,20,20));
                GAME_LOG("Du bullet hits player for %d damage!", dmg);

                // 移除子弹並结束发射等待
                bulletNode->unschedule(DU_BULLET_SCHEDULE_KEY);
                if (bulletNode->getParent()) bulletNode->removeFromParent();

                this->_currentBullet = nullptr;
                this->_isFiring = false;
                // 恢复攻击状态为空闲
                if (this->_currentState == EntityState::ATTACK) setState(EntityState::IDLE);
                return;
            }
        }

        // 2) 检测房间边界（若设置了房间边界，子弹出界则消失）
        if (_hasRoomBounds)
        {
            Vec2 bpos;
            if (bulletNode->getParent())
                bpos = bulletNode->getParent()->convertToNodeSpace(bulletWorldPos);
            else
                bpos = bulletNode->getPosition();

            if (bpos.x < _roomBounds.getMinX() || bpos.x > _roomBounds.getMaxX()
                || bpos.y < _roomBounds.getMinY() || bpos.y > _roomBounds.getMaxY())
            {
                bulletNode->unschedule(DU_BULLET_SCHEDULE_KEY);
                if (bulletNode->getParent()) bulletNode->removeFromParent();
                this->_currentBullet = nullptr;
                this->_isFiring = false;
                if (this->_currentState == EntityState::ATTACK) setState(EntityState::IDLE);
                return;
            }
        }

        // 3) 检测与障碍（tag == Constants::Tag::WALL）碰撞
        // 更全面地遍历子节点（考虑房间层次结构：room -> barrier sprite）
        Node* p = bulletNode->getParent();
        if (p)
        {
            // 碰撞检测半径：使用地砖大小为基准
            float wallCollisionRadius = Constants::FLOOR_TILE_SIZE * 0.9f;

            // 遍历第一层子节点（通常 room 节点在 gameLayer 下）
            for (auto child : p->getChildren())
            {
                if (!child) continue;

                // 直接子节点为墙（有时障碍直接挂在 gameLayer）
                if (child->getTag() == Constants::Tag::WALL)
                {
                    Vec2 wallWorldPos = child->getParent()->convertToWorldSpace(child->getPosition());
                    if (bulletWorldPos.distance(wallWorldPos) < wallCollisionRadius)
                    {
                        bulletNode->unschedule(DU_BULLET_SCHEDULE_KEY);
                        if (bulletNode->getParent()) bulletNode->removeFromParent();
                        this->_currentBullet = nullptr;
                        this->_isFiring = false;
                        if (this->_currentState == EntityState::ATTACK) setState(EntityState::IDLE);
                        GAME_LOG("Du bullet destroyed by barrier collision (direct child)");
                        return;
                    }
                }

                // 检查 child 的子节点（例如 room 下的 barrier sprite）
                for (auto subChild : child->getChildren())
                {
                    if (!subChild) continue;

                    if (subChild->getTag() == Constants::Tag::WALL)
                    {
                        Vec2 wallWorldPos = subChild->getParent()->convertToWorldSpace(subChild->getPosition());
                        if (bulletWorldPos.distance(wallWorldPos) < wallCollisionRadius)
                        {
                            bulletNode->unschedule(DU_BULLET_SCHEDULE_KEY);
                            if (bulletNode->getParent()) bulletNode->removeFromParent();
                            this->_currentBullet = nullptr;
                            this->_isFiring = false;
                            if (this->_currentState == EntityState::ATTACK) setState(EntityState::IDLE);
                            GAME_LOG("Du bullet destroyed by barrier collision (nested child)");
                            return;
                        }
                    }

                    // 再往深一层检查（少数层级有三层）
                    for (auto deepChild : subChild->getChildren())
                    {
                        if (!deepChild) continue;
                        if (deepChild->getTag() == Constants::Tag::WALL)
                        {
                            Vec2 wallWorldPos = deepChild->getParent()->convertToWorldSpace(deepChild->getPosition());
                            if (bulletWorldPos.distance(wallWorldPos) < wallCollisionRadius)
                            {
                                bulletNode->unschedule(DU_BULLET_SCHEDULE_KEY);
                                if (bulletNode->getParent()) bulletNode->removeFromParent();
                                this->_currentBullet = nullptr;
                                this->_isFiring = false;
                                if (this->_currentState == EntityState::ATTACK) setState(EntityState::IDLE);
                                GAME_LOG("Du bullet destroyed by barrier collision (deep nested)");
                                return;
                            }
                        }
                    }
                }
            }
        }

        // 其他逻辑可加入（例如穿透、反弹等）

    }, DU_BULLET_SCHEDULE_KEY);

    // 子弹移动动作（按速度计算飞行时间，若 DU_BULLET_SPEED <= 0 则回退到固定时间）
    float distance = startPos.distance(targetPos);
    float duration = (DU_BULLET_SPEED > 0.0f) ? (distance / DU_BULLET_SPEED) : DU_BULLET_FLIGHT_TIME;
    // 限制最小/最大时长，避免极端值
    duration = std::max(0.05f, std::min(duration, 10.0f));
    auto moveTo = MoveTo::create(duration, targetPos);
    auto remove = CallFunc::create([this, bulletNode]() {
        // 飞行时间结束 -> 移除子弹（若仍存在），结束发射状态
        bulletNode->unschedule(DU_BULLET_SCHEDULE_KEY);
        if (bulletNode->getParent()) bulletNode->removeFromParent();
        this->_currentBullet = nullptr;
        this->_isFiring = false;
        if (this->_currentState == EntityState::ATTACK) setState(EntityState::IDLE);
    });
    auto seq = Sequence::create(moveTo, remove, nullptr);
    bulletNode->runAction(seq);
}

void Du::playAttackAnimation()
{
    if (_attackAnimation && _sprite)
    {
        // 停止移动循环动作（仅移动，不影响其他动画）
        auto moveAct = _sprite->getActionByTag(DU_MOVE_ACTION_TAG);
        if (moveAct) _sprite->stopAction(moveAct);

        _attackAnimation->setRestoreOriginalFrame(true);
        auto animate = Animate::create(_attackAnimation);
        animate->setTag(DU_WINDUP_ACTION_TAG);
        _sprite->runAction(animate);
    }
}

void Du::die()
{
    // 先调用基类逻辑（处理红色标记 / KongKaZi 等）
    Enemy::die();

    // 防重入
    if (_currentState == EntityState::DIE && !_isAlive) return;
    setState(EntityState::DIE);
    _isAlive = false;

    // 停止动作并播放死亡动画
    this->stopAllActions();
    if (_sprite) { _sprite->stopAllActions(); _sprite->setVisible(true); _sprite->setOpacity(255); }

    auto finalizeRemove = [this]() {
        // 如果还有未决子弹，移除并清理
        if (_currentBullet)
        {
            _currentBullet->unschedule(DU_BULLET_SCHEDULE_KEY);
            if (_currentBullet->getParent()) _currentBullet->removeFromParent();
            _currentBullet = nullptr;
        }
        this->removeFromParent();
    };

    if (_dieAnimation && _sprite)
    {
        auto animate = Animate::create(_dieAnimation);
        auto fade = FadeOut::create(0.4f);
        auto seq = Sequence::create(animate, fade, CallFunc::create(finalizeRemove), nullptr);
        _sprite->runAction(seq);
        return;
    }

    finalizeRemove();
}

/**
 * 覆写移动：当处于 ATTACK/发射中或死亡时禁止移动，并控制移动动画播放/停止。
 * 实现风格与 Ayao::move 保持一致，同时考虑 _isFiring（发射期间不移动）。
 */
void Du::move(const Vec2& direction, float dt)
{
    // 在攻击阶段或发射期间禁止移动（确保攻击前摇/子弹过程不被移动覆盖）
    if (_currentState == EntityState::ATTACK || _currentState == EntityState::DIE || _isFiring)
    {
        Character::move(Vec2::ZERO, dt);
        return;
    }

    // 小阈值判断为“不移动”
    const float STOP_THRESHOLD_SQ = 1.0f;
    if (direction.lengthSquared() <= STOP_THRESHOLD_SQ)
    {
        // 停止移动动作（如果存在）
        if (_sprite)
        {
            auto act = _sprite->getActionByTag(DU_MOVE_ACTION_TAG);
            if (act)
            {
                _sprite->stopAction(act);
            }

            // 恢复到移动动画的第一帧（通常为站立/待机状态）
            if (_moveAnimation)
            {
                auto frames = _moveAnimation->getFrames();
                if (!frames.empty())
                {
                    auto firstFrame = frames.front()->getSpriteFrame();
                    if (firstFrame)
                    {
                        _sprite->setSpriteFrame(firstFrame);
                    }
                }
            }
        }

        // 调用基类移动以保持状态一致
        Character::move(Vec2::ZERO, dt);
        return;
    }

    // 有移动向量：先调用基类以实际移动实体
    Vec2 dirNorm = direction.getNormalized();
    Character::move(dirNorm, dt);

    // 设置朝向（左右），并确保移动动画在循环播放
    if (_sprite && _moveAnimation)
    {
        _sprite->setFlippedX(dirNorm.x < 0.0f);

        // 如果移动动画未在播放，则启动循环播放并打上 tag
        if (!_sprite->getActionByTag(DU_MOVE_ACTION_TAG))
        {
            auto animate = Animate::create(_moveAnimation);
            auto repeat = RepeatForever::create(animate);
            repeat->setTag(DU_MOVE_ACTION_TAG);
            _sprite->runAction(repeat);
        }
    }
}

void Du::setRoomBounds(const Rect& bounds)
{
    _roomBounds = bounds;
    _hasRoomBounds = true;
}