#include "TangHuang.h"
#include "cocos2d.h"
#include "Entities/Player/Player.h"
#include "Entities/Enemy/IronLightCup.h"
#include "Scenes/GameScene.h"

USING_NS_CC;

static const int TANGHUANG_MOVE_ACTION_TAG = 0xF201;
static const int TANGHUANG_WINDUP_ACTION_TAG = 0xF202;

// 新增：烟雾常量
static constexpr float TANGHUANG_SMOKE_DELAY = 0.5f;    // 0.5s 延迟释放
static constexpr float TANGHUANG_SMOKE_DURATION = 5.0f; // 持续 5s
static constexpr float TANGHUANG_SMOKE_RADIUS = 250.0f; // 半径 150

TangHuang::TangHuang()
    : _moveAnimation(nullptr)
    , _attackAnimation(nullptr)
    , _dieAnimation(nullptr)
    , _skillAnimation(nullptr)
    , _roomBounds(Rect::ZERO)
    , _hasRoomBounds(false)
    , _attackTarget(nullptr)
    , _skillCooldown(15.0f)      // 默认技能冷却 15s
    , _skillCooldownTimer(0.0f)
{
}

TangHuang::~TangHuang()
{
    CC_SAFE_RELEASE(_moveAnimation);
    CC_SAFE_RELEASE(_attackAnimation);
    CC_SAFE_RELEASE(_dieAnimation);
    CC_SAFE_RELEASE(_skillAnimation);
}

bool TangHuang::init()
{
    if (!Enemy::init()) return false;

    setEnemyType(EnemyType::MELEE);

    setupAttributes();
    loadAnimations();

    // 如果有移动动画，使用第一帧作为初始精灵
    if (_moveAnimation)
    {
        auto frames = _moveAnimation->getFrames();
        if (!frames.empty())
        {
            auto sprite = Sprite::createWithSpriteFrame(frames.front()->getSpriteFrame());
            this->bindSprite(sprite);
            // 将堂皇精灵整体放大 1.5 倍
            if (this->_sprite) this->_sprite->setScale(1.8f);
        }
    }

    GAME_LOG("TangHuang initialized");
    return true;
}

void TangHuang::setupAttributes()
{
    // 默认属性（可根据游戏平衡调整）
    setMaxHP(1600);
    setHP(1600);

    setAttack(50);
    setMoveSpeed(70.0f);

    setSightRange(200.0f);
    setAttackRange(40.0f);
    setAttackCooldown(1.8f);
    setAttackWindup(0.5f);

    // 技能冷却：15 秒（明确写在这里以便一目了然）
    setSkillCooldown(15.0f);
    _skillCooldownTimer = 0.0f;
}

void TangHuang::loadAnimations()
{
    // 通用加载函数风格与项目其他类一致：先尝试通过文件创建 Sprite 再 fallback 到 SpriteFrameCache
    Vector<SpriteFrame*> moveFrames;
    for (int i = 1; i <= 8; ++i)
    {
        char filename[256];
        sprintf(filename, "Enemy/TangHuang&&Iron LightCup/TangHuang/TangHuang_Move/TangHuang_Move_%04d.png", i);

        SpriteFrame* frame = nullptr;
        auto s = Sprite::create(filename);
        if (s) frame = s->getSpriteFrame();

        if (!frame)
        {
            char basename[128];
            sprintf(basename, "TangHuang_Move_%04d.png", i);
            frame = SpriteFrameCache::getInstance()->getSpriteFrameByName(basename);
        }

        if (frame) moveFrames.pushBack(frame);
        else GAME_LOG("TangHuang: failed to load move frame: %s", filename);
    }

    if (!moveFrames.empty())
    {
        _moveAnimation = Animation::createWithSpriteFrames(moveFrames, 0.12f);
        _moveAnimation->retain();
    }

    Vector<SpriteFrame*> attackFrames;
    for (int i = 1; i <= 6; ++i)
    {
        char filename[256];
        sprintf(filename, "Enemy/TangHuang&&Iron LightCup/TangHuang/TangHuang_Attack/TangHuang_Attack_%04d.png", i);

        SpriteFrame* frame = nullptr;
        auto s = Sprite::create(filename);
        if (s) frame = s->getSpriteFrame();

        if (!frame)
        {
            char basename[128];
            sprintf(basename, "TangHuang_Attack_%04d.png", i);
            frame = SpriteFrameCache::getInstance()->getSpriteFrameByName(basename);
        }

        if (frame) attackFrames.pushBack(frame);
        else GAME_LOG("TangHuang: failed to load attack frame: %s", filename);
    }

    if (!attackFrames.empty())
    {
        _attackAnimation = Animation::createWithSpriteFrames(attackFrames, 0.10f);
        _attackAnimation->retain();
    }

    // 新增：加载 Skill 动画（路径与其他动画一致，只将 Move 改为 Skill）
    Vector<SpriteFrame*> skillFrames;
    for (int i = 1; i <= 8; ++i) 
    {
        char filename[256];
        sprintf(filename, "Enemy/TangHuang&&Iron LightCup/TangHuang/TangHuang_Skill/TangHuang_Skill_%04d.png", i);

        SpriteFrame* frame = nullptr;
        auto s = Sprite::create(filename);
        if (s) frame = s->getSpriteFrame();

        if (!frame)
        {
            char basename[128];
            sprintf(basename, "TangHuang_Skill_%04d.png", i);
            frame = SpriteFrameCache::getInstance()->getSpriteFrameByName(basename);
        }

        if (frame) skillFrames.pushBack(frame);
        else GAME_LOG("TangHuang: failed to load skill frame: %s", filename);
    }

    if (!skillFrames.empty())
    {
        _skillAnimation = Animation::createWithSpriteFrames(skillFrames, 0.10f);
        _skillAnimation->retain();
    }

    Vector<SpriteFrame*> dieFrames;
    for (int i = 1; i <= 5; ++i)
    {
        char filename[256];
        sprintf(filename, "Enemy/TangHuang&&Iron LightCup/TangHuang/TangHuang_Die/TangHuang_Die_%04d.png", i);

        SpriteFrame* frame = nullptr;
        auto s = Sprite::create(filename);
        if (s) frame = s->getSpriteFrame();

        if (!frame)
        {
            char basename[128];
            sprintf(basename, "TangHuang_Die_%04d.png", i);
            frame = SpriteFrameCache::getInstance()->getSpriteFrameByName(basename);
        }

        if (frame) dieFrames.pushBack(frame);
        else GAME_LOG("TangHuang: failed to load die frame: %s", filename);
    }

    if (!dieFrames.empty())
    {
        _dieAnimation = Animation::createWithSpriteFrames(dieFrames, 0.12f);
        _dieAnimation->retain();
    }

    // 如果已绑定 sprite，确保起始帧存在（如果没有绑定，init() 会用 move 的第一帧绑定）
    if (_sprite == nullptr && _moveAnimation && !_moveAnimation->getFrames().empty())
    {
        auto sprite = Sprite::createWithSpriteFrame(_moveAnimation->getFrames().front()->getSpriteFrame());
        this->bindSprite(sprite);
        // 将堂皇精灵整体放大 1.5 倍（避免遗漏任何绑定分支）
        if (this->_sprite) this->_sprite->setScale(1.8f);
    }

    GAME_LOG("TangHuang animations loaded");
}

void TangHuang::update(float dt)
{
    Enemy::update(dt);

    // 技能冷却计时
    if (_skillCooldownTimer > 0.0f)
    {
        _skillCooldownTimer = std::max(0.0f, _skillCooldownTimer - dt);
    }

    // 如果设置了房间边界，则限制位置在房间内（与 XinXing 保持一致）
    if (_hasRoomBounds)
    {
        Vec2 pos = this->getPosition();
        pos.x = std::max(_roomBounds.getMinX(), std::min(pos.x, _roomBounds.getMaxX()));
        pos.y = std::max(_roomBounds.getMinY(), std::min(pos.y, _roomBounds.getMaxY()));
        this->setPosition(pos);
    }

    // 可以放置特殊状态更新（如被动技能、冷却等）
}

void TangHuang::executeAI(Player* player, float dt)
{
    if (!player)
    {
        _attackTarget = nullptr;
        return;
    }

    // 如果技能可用，则优先停止并释放技能（不继续追击）
    if (canUseSkill() && _currentState != EntityState::ATTACK && _currentState != EntityState::DIE)
    {
        // 停止移动并释放技能（useSmokeSkill 内会处理延迟与冷却）
        move(Vec2::ZERO, dt); // 停止移动，保持状态一致
        faceToPosition(player->getPosition());
        useSmokeSkill();
        return;
    }

    // 设置当前攻击目标引用，攻击回调会使用此引用（非 owning）
    if (isPlayerInSight(player))
    {
        _attackTarget = player;

        if (isPlayerInAttackRange(player))
        {
            attack();
        }
        else
        {
            chasePlayer(player, dt);
        }
    }
    else
    {
        _attackTarget = nullptr;
        patrol(dt);
    }
}

void TangHuang::move(const cocos2d::Vec2& direction, float dt)
{
    if (!_sprite) return;

    // 面向运动方向
    if (direction != Vec2::ZERO)
    {
        faceToPosition(this->getPosition() + direction);
    }

    // 移动位置（调用基类以处理碰撞/状态/速度）
    Enemy::move(direction, dt);

    // 如果已有移动动画且未在播放，则开始播放
    auto existing = _sprite->getActionByTag(TANGHUANG_MOVE_ACTION_TAG);
    if (!_moveAnimation)
    {
        return;
    }

    if (!existing)
    {
        auto animate = Animate::create(_moveAnimation);
        auto repeat = RepeatForever::create(animate);
        repeat->setTag(TANGHUANG_MOVE_ACTION_TAG);
        _sprite->runAction(repeat);
    }
}

void TangHuang::attack()
{
    if (!canAttack()) return;

    setState(EntityState::ATTACK);
    resetAttackCooldown();

    // 停止移动循环（只停止移动 action）
    if (_sprite)
    {
        auto moveAct = _sprite->getActionByTag(TANGHUANG_MOVE_ACTION_TAG);
        if (moveAct) _sprite->stopAction(moveAct);
    }

    // 播放前摇攻击动画（若有）
    if (_attackAnimation && _sprite)
    {
        _attackAnimation->setRestoreOriginalFrame(true);
        auto animate = Animate::create(_attackAnimation);
        animate->setTag(TANGHUANG_WINDUP_ACTION_TAG);
        _sprite->runAction(animate);

        float windup = this->getAttackWindup();
        // 捕获当前目标指针，在 windup 完成时执行实际伤害
        Player* target = _attackTarget;
        auto delay = DelayTime::create(windup);
        auto callback = CallFunc::create([this, target]() {
            if (_currentState == EntityState::ATTACK)
            {
                // 触发实际伤害判定
                if (target && !target->isDead() && isPlayerInAttackRange(target))
                {
                    this->attackPlayer(target);
                }
                setState(EntityState::IDLE);
            }
        });
        auto seq = Sequence::create(delay, callback, nullptr);
        this->runAction(seq);
    }
    else
    {
        // 无动画时仅按 windup 恢复状态并造成伤害
        float windup = this->getAttackWindup();
        Player* target = _attackTarget;
        auto delay = DelayTime::create(windup);
        auto cb = CallFunc::create([this, target]() {
            if (_currentState == EntityState::ATTACK)
            {
                if (target && !target->isDead() && isPlayerInAttackRange(target))
                {
                    this->attackPlayer(target);
                }
                setState(EntityState::IDLE);
            }
        });
        this->runAction(Sequence::create(delay, cb, nullptr));
    }

    GAME_LOG("TangHuang attacks!");
}

void TangHuang::attackPlayer(Player* player)
{
    if (!player) return;

    int dmg = this->getAttack();
    player->takeDamage(dmg);

    GAME_LOG("TangHuang dealt %d damage to player", dmg);

    // 播放攻击动画（如果需要与伤害分离）
    playAttackAnimation();

    // 使用技能（内部会检查冷却并执行延迟）
    useSmokeSkill();
}

void TangHuang::useSmokeSkill()
{
    if (!canUseSkill()) return;

    // 开始冷却
    _skillCooldownTimer = _skillCooldown;

    // 延迟实际释放（0.5s）
    this->runAction(Sequence::create(DelayTime::create(TANGHUANG_SMOKE_DELAY),
        CallFunc::create([this]() {
            this->spawnSmoke();
        }), nullptr));
}

void TangHuang::spawnSmoke()
{
    // 在释放技能时播放技能动画（若存在）
    if (_skillAnimation && _sprite)
    {
        _sprite->stopActionByTag(TANGHUANG_MOVE_ACTION_TAG);
        auto animate = Animate::create(_skillAnimation);
        animate->setTag(TANGHUANG_WINDUP_ACTION_TAG);
        _sprite->runAction(animate);
    }

    // 创建烟雾 DrawNode（视觉）并放在地板之上（而非角色下方）
    auto smoke = DrawNode::create();
    Color4F smokeColor(0.85f, 0.85f, 0.85f, 0.6f); // 浅灰白，半透明
    smoke->drawSolidCircle(Vec2::ZERO, TANGHUANG_SMOKE_RADIUS, 0, 64, smokeColor);

    // 将烟雾添加到与角色相同的父节点（通常为 gameLayer），并设置在地板之上
    Node* parent = this->getParent();
    if (parent)
    {
        int smokeZ = Constants::ZOrder::FLOOR + 1; // 放在地板之上
        parent->addChild(smoke, smokeZ);
        smoke->setPosition(this->getPosition());
        smoke->setGlobalZOrder(static_cast<float>(smokeZ));
    }
    else
    {
        // 退回兼容行为：附加到 self（仍然可工作）
        this->addChild(smoke, -1);
        smoke->setPosition(Vec2::ZERO);
    }

    // 用一个堆上分配的容器记录当前圆内的敌人（用于差分 add/remove）
    auto insideVec = new std::vector<Enemy*>();

    // 每帧更新：检查父节点（通常为 gameLayer）下的敌人，添加/移除 stealth 源
    smoke->schedule([this, smoke, insideVec](float dt) {
        // 使烟雾一直跟随堂皇移动（保证即便父节点在 world 空间，位置始终同步）
        if (smoke->getParent())
        {
            smoke->setPosition(this->getPosition());
        }

        Node* parentLocal = this->getParent();
        if (!parentLocal) return;

        std::vector<Enemy*> currentInside;
        for (auto child : parentLocal->getChildren())
        {
            if (child && child->getTag() == Constants::Tag::ENEMY)
            {
                auto e = dynamic_cast<Enemy*>(child);
                if (!e) continue;
                if (e->isDead()) continue;
                float dist = e->getPosition().distance(this->getPosition());
                if (dist <= TANGHUANG_SMOKE_RADIUS)
                {
                    currentInside.push_back(e);
                }
            }
        }

        // 对新进入的敌人添加源（使用 smoke 指针 作为 source id）
        for (auto e : currentInside)
        {
            if (std::find(insideVec->begin(), insideVec->end(), e) == insideVec->end())
            {
                e->addStealthSource((void*)smoke);
            }
        }

        // 对已离开的敌人移除源
        for (auto prev : *insideVec)
        {
            if (std::find(currentInside.begin(), currentInside.end(), prev) == currentInside.end())
            {
                prev->removeStealthSource((void*)smoke);
            }
        }

        // 更新 insideVec
        insideVec->assign(currentInside.begin(), currentInside.end());

    }, "TangHuangSmokeUpdate");

    // After duration, unschedule update, remove stealth from remaining inside, and remove node
    smoke->runAction(Sequence::create(DelayTime::create(TANGHUANG_SMOKE_DURATION),
        CallFunc::create([smoke, insideVec]() {
            // 停止调度
            smoke->unschedule("TangHuangSmokeUpdate");

            // 移除残留的 stealth 源
            for (auto e : *insideVec)
            {
                if (e) e->removeStealthSource((void*)smoke);
            }

            // 清理容器
            delete insideVec;

            // 淡出并移除烟雾节点
            auto fade = FadeOut::create(0.35f);
            auto remove = CallFunc::create([smoke]() {
                if (smoke->getParent()) smoke->removeFromParent();
            });
            smoke->runAction(Sequence::create(fade, remove, nullptr));
        }), nullptr));
}

void TangHuang::playAttackAnimation()
{
    // 额外的视觉效果或同步逻辑可以放这里（若攻击与动画分开驱动）
    if (_attackAnimation && _sprite)
    {
        _sprite->stopActionByTag(TANGHUANG_MOVE_ACTION_TAG);
        auto animate = Animate::create(_attackAnimation);
        animate->setTag(TANGHUANG_WINDUP_ACTION_TAG);
        _sprite->runAction(animate);
    }
}

void TangHuang::die()
{
    // 先调用基类以触发红色标记 / KongKaZi 等通用逻辑
    Enemy::die();

    // 防重入：设置死亡状态
    if (_currentState == EntityState::DIE && !_isAlive) return;
    setState(EntityState::DIE);
    _isAlive = false;

    // 停止其他动作并播放死亡动画，播放完成后生成 1 个 IronLightCup（与 XinXing 的实现一致）
    this->stopAllActions();
    if (_sprite) { _sprite->stopAllActions(); _sprite->setVisible(true); _sprite->setOpacity(255); }

    auto finalizeSpawn = [this]() {
        Vec2 basePos = this->getPosition();

        // 创建并放置 1 个 IronLightCup，位置在堂皇原地稍微偏移随机一点
        auto il = IronLightCup::create();
        if (!il) {
            this->removeFromParent();
            return;
        }

        float angle = CCRANDOM_MINUS1_1() * M_PI;
        float r = 12.0f;
        Vec2 spawnPos = basePos + Vec2(std::cos(angle) * r, std::sin(angle) * r);
        il->setPosition(spawnPos);

        if (_hasRoomBounds) il->setRoomBounds(_roomBounds);

        // 尝试通过当前运行场景注册到 GameScene，以便被 update/enemies 管理（与 XinXing 中的逻辑完全一致）
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
        if (gs) gs->addEnemy(il);
        else if (running) running->addChild(il);

        // 移除自身（与 XinXing 保持相同的移除调用风格）
        this->removeFromParent();
    };

    if (_dieAnimation && _sprite)
    {
        auto animate = Animate::create(_dieAnimation);
        auto fade = FadeOut::create(0.4f);
        auto seq = Sequence::create(animate, fade, CallFunc::create(finalizeSpawn), nullptr);
        _sprite->runAction(seq);
        return;
    }

    // 没有死亡动画则直接生成并移除
    finalizeSpawn();
}