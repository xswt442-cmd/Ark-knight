#include "KuiLongBoss.h"
#include "cocos2d.h"
#include "Entities/Player/Player.h"
#include "ui/CocosGUI.h"

USING_NS_CC;

static const char* LOG_TAG = "KuiLongBoss";

// 构造函数：在构造体内确保 overlay 指针初始化
KuiLongBoss::KuiLongBoss()
    : _phase(PHASE_A)
    , _phaseTimer(0.0f)
    , _phaseADuration(30.0f)
    , _animAIdle(nullptr)
    , _animAChangeToB(nullptr)
    , _animBMove(nullptr)
    , _animBAttack(nullptr)
    , _animBChangeToC(nullptr)
    , _animBChengWuJie(nullptr)
    , _skillSprite(nullptr)
    , _moveAnimPlaying(false)
    , _bossHPBar(nullptr)
    , _bossHPLabel(nullptr)
    , _bossBarOffsetY(8.0f)
    , _skillCooldown(15.0f)
    , _skillCooldownTimer(_skillCooldown) // 初始允许使用（如果希望首次不可用，把这里设为 0.0f）
    , _skillRange(220.0f) // 比默认攻击范围大（默认 120），按需调整
    , _skillDamagePerHit(getAttack() * 4) // 每次高额伤害（基于攻击力的倍数，可调）
    , _skillPlaying(false)
{
}

KuiLongBoss::~KuiLongBoss()
{
    if (_animAIdle) { _animAIdle->release(); _animAIdle = nullptr; }
    if (_animAChangeToB) { _animAChangeToB->release(); _animAChangeToB = nullptr; }
    if (_animBMove) { _animBMove->release(); _animBMove = nullptr; }
    if (_animBAttack) { _animBAttack->release(); _animBAttack = nullptr; }
    if (_animBChangeToC) { _animBChangeToC->release(); _animBChangeToC = nullptr; }
    if (_animBChengWuJie) { _animBChengWuJie->release(); _animBChengWuJie = nullptr; }

    // 移除并释放用于播放技能动画的覆盖精灵（若存在）
    if (_skillSprite)
    {
        _skillSprite->removeFromParentAndCleanup(true);
        _skillSprite = nullptr;
    }

    // UI 子节点由 Node 管理，不需要手动 release
}

bool KuiLongBoss::init()
{
    if (!Enemy::init()) return false;

    // 基础属性（可根据需求调整）
    setEnemyType(EnemyType::MELEE);
    setMaxHP(20000);
    setHP(getMaxHP());
    setMoveSpeed(60.0f);
    setAttack(80);
    setSightRange(1000.0f);
    setAttackRange(120.0f);

    // 攻击间隔：1 秒（Boss 的攻击冷却），通过 Character::setAttackCooldown / getAttackCooldown 管理
    setAttackCooldown(2.0f);
    // 攻击前摇时长（和 Ayao 类似，可按需调整）
    setAttackWindup(0.1f);

    // 阶段初始
    _phase = PHASE_A;
    _phaseTimer = 0.0f;
    _phaseADuration = 10.0f;

    // 载入动画资源（使用 Ayao 风格逐帧加载，便于检查）
    loadAnimations();

    // 绑定初始精灵：优先 A Idle 第一帧
    if (_animAIdle)
    {
        auto frames = _animAIdle->getFrames();
        if (!frames.empty())
        {
            auto sprite = Sprite::createWithSpriteFrame(frames.front()->getSpriteFrame());
            this->bindSprite(sprite);
        }
    }
    else
    {
        // 兜底占位
        auto sprite = Sprite::create();
        sprite->setTextureRect(Rect(0, 0, 48, 48));
        sprite->setColor(Color3B::RED);
        this->bindSprite(sprite);
    }

    // 播放 A Idle 循环（若存在）
    if (_sprite && _animAIdle)
    {
        auto animate = Animate::create(_animAIdle);
        auto repeat = RepeatForever::create(animate);
        repeat->setTag(KUI_LONG_MOVE_TAG);
        _sprite->runAction(repeat);
        _moveAnimPlaying = true;
    }

    // 出生在屏幕/场景中心（尝试 GameScene 居中；若失败则用可见区域中心）
    Vec2 center = Vec2::ZERO;
    Size vs = Director::getInstance()->getVisibleSize();
    center = Vec2(vs.width * 0.5f, vs.height * 0.5f);
    this->setPosition(center);

    // ---------- 在 PHASE_A 时使用 TangHuang 一样的 Stealth 源，避免被子弹锁定 ----------
    this->addStealthSource((void*)this);

    // ---------- 创建 Boss 血条 UI（精灵下方） ----------
    {
        float barWidth = 120.0f;
        float barHeight = 10.0f;

        // 我们把血条设为 world-space 的渲染层级：在地板层之上，但不属于 UI 层（避免被 UI 遮挡）
        float bgGlobalZ = static_cast<float>(Constants::ZOrder::FLOOR + 1);
        float fillGlobalZ = static_cast<float>(Constants::ZOrder::FLOOR + 2);
        float labelGlobalZ = static_cast<float>(Constants::ZOrder::FLOOR + 3);

        auto bg = Sprite::create("UIs/StatusBars/Bars/EmplyBar.png");
        if (bg)
        {
            bg->setAnchorPoint(Vec2(0.5f, 0.5f));
            bg->setScaleX(barWidth / bg->getContentSize().width);
            bg->setScaleY(barHeight / bg->getContentSize().height);
            bg->setPosition(Vec2(0.0f, -(_sprite ? _sprite->getBoundingBox().size.height * 0.5f : 24.0f) - _bossBarOffsetY));
            // 把血条放在地板层之上（world-space）
            bg->setGlobalZOrder(bgGlobalZ);
            // 同时把它当作 Boss 的子节点，以便随 Boss 移动
            this->addChild(bg, 0);
        }

        _bossHPBar = cocos2d::ui::LoadingBar::create("UIs/StatusBars/Bars/HealthFill.png");
        if (_bossHPBar)
        {
            _bossHPBar->setPercent(100.0f);
            _bossHPBar->setAnchorPoint(Vec2(0.5f, 0.5f));
            _bossHPBar->setScaleX(barWidth / _bossHPBar->getContentSize().width);
            _bossHPBar->setScaleY(barHeight / _bossHPBar->getContentSize().height);
            _bossHPBar->setPosition(Vec2(0.0f, -(_sprite ? _sprite->getBoundingBox().size.height * 0.5f : 24.0f) - _bossBarOffsetY));
            _bossHPBar->setGlobalZOrder(fillGlobalZ);
            this->addChild(_bossHPBar, 1);
        }

        char buf[64];
        sprintf(buf, "%d/%d", getHP(), getMaxHP());
        _bossHPLabel = Label::createWithSystemFont(buf, "Arial", 12);
        if (_bossHPLabel)
        {
            _bossHPLabel->setAnchorPoint(Vec2(0.5f, 0.5f));
            _bossHPLabel->setTextColor(Color4B::WHITE);
            _bossHPLabel->setPosition(Vec2(0.0f, -(_sprite ? _sprite->getBoundingBox().size.height * 0.5f : 24.0f) - _bossBarOffsetY - 12.0f));
            _bossHPLabel->setGlobalZOrder(labelGlobalZ);
            this->addChild(_bossHPLabel, 2);
        }
    }

    GAME_LOG("%s initialized at center (HP=%d) PhaseA duration=%.1f", LOG_TAG, getHP(), _phaseADuration);

    return true;
}

void KuiLongBoss::loadAnimations()
{
    // 按 Ayao 的写法逐动画逐帧加载，便于人工检查资源是否存在并记录日志

    // A Idle
    {
        Vector<SpriteFrame*> frames;
        for (int i = 1; i <= 8; ++i)
        {
            char filename[512];
            sprintf(filename, "Enemy/_BOSS_KuiLong/Boss_A_Idle/KL_A_Idle_%04d.png", i);
            SpriteFrame* frame = nullptr;
            auto s = Sprite::create(filename);
            if (s) frame = s->getSpriteFrame();
            if (!frame)
            {
                char basename[256];
                sprintf(basename, "KL_A_Idle_%04d.png", i);
                frame = SpriteFrameCache::getInstance()->getSpriteFrameByName(basename);
            }
            if (frame) frames.pushBack(frame);
            else break;
        }
        if (!frames.empty())
        {
            _animAIdle = Animation::createWithSpriteFrames(frames, 0.12f);
            _animAIdle->retain();
        }
    }

    // A -> B 转场
    {
        Vector<SpriteFrame*> frames;
        for (int i = 1; i <= 12; ++i)
        {
            char filename[512];
            sprintf(filename, "Enemy/_BOSS_KuiLong/Boss_A_ChangeToB/KL_A_ChangeToB_%04d.png", i);
            SpriteFrame* frame = nullptr;
            auto s = Sprite::create(filename);
            if (s) frame = s->getSpriteFrame();
            if (!frame)
            {
                char basename[256];
                sprintf(basename, "KL_A_ChangeToB_%04d.png", i);
                frame = SpriteFrameCache::getInstance()->getSpriteFrameByName(basename);
            }
            if (frame) frames.pushBack(frame);
            else break;
        }
        if (!frames.empty())
        {
            _animAChangeToB = Animation::createWithSpriteFrames(frames, 0.08f);
            _animAChangeToB->retain();
        }
    }

    // B Move
    {
        Vector<SpriteFrame*> frames;
        for (int i = 1; i <= 7; ++i)
        {
            char filename[512];
            sprintf(filename, "Enemy/_BOSS_KuiLong/Boss_B_Move/KL_B_Move_%04d.png", i);
            SpriteFrame* frame = nullptr;
            auto s = Sprite::create(filename);
            if (s) frame = s->getSpriteFrame();
            if (!frame)
            {
                char basename[256];
                sprintf(basename, "KL_B_Move_%04d.png", i);
                frame = SpriteFrameCache::getInstance()->getSpriteFrameByName(basename);
            }
            if (frame) frames.pushBack(frame);
            else break;
        }
        if (!frames.empty())
        {
            _animBMove = Animation::createWithSpriteFrames(frames, 0.10f);
            _animBMove->retain();
        }
    }

    // B Attack
    {
        Vector<SpriteFrame*> frames;
        for (int i = 1; i <= 12; ++i)
        {
            char filename[512];
            sprintf(filename, "Enemy/_BOSS_KuiLong/Boss_B_Attack/KL_B_Attack_%04d.png", i);
            SpriteFrame* frame = nullptr;
            auto s = Sprite::create(filename);
            if (s) frame = s->getSpriteFrame();

            if (frame) frames.pushBack(frame);
            else break;
        }
        if (!frames.empty())
        {
            _animBAttack = Animation::createWithSpriteFrames(frames, 0.10f);
            _animBAttack->retain();
        }
    }

    // B -> C 死亡转换动画
    {
        Vector<SpriteFrame*> frames;
        for (int i = 1; i <= 13; ++i)
        {
            char filename[512];
            sprintf(filename, "Enemy/_BOSS_KuiLong/Boss_B_ChangeToC/KL_B_ChangeTo_C_%04d.png", i);
            SpriteFrame* frame = nullptr;
            auto s = Sprite::create(filename);
            if (s) frame = s->getSpriteFrame();
            if (!frame)
            {
                char basename[256];
                sprintf(basename, "KL_B_ChangeTo_C_%04d.png", i);
                frame = SpriteFrameCache::getInstance()->getSpriteFrameByName(basename);
            }
            if (frame) frames.pushBack(frame);
            else break;
        }
        if (!frames.empty())
        {
            _animBChangeToC = Animation::createWithSpriteFrames(frames, 0.10f);
            _animBChangeToC->retain();
        }
    }

    // B ChengWuJie 技能（23 帧）
    {
        Vector<SpriteFrame*> frames;
        for (int i = 1; i <= 23; ++i)
        {
            char filename[512];
            sprintf(filename, "Enemy/_BOSS_KuiLong/Boss_B_Skill2ChengWuJie/KL_B_Skill2_%04d.png", i);
            SpriteFrame* frame = nullptr;
            auto s = Sprite::create(filename);
            if (s) frame = s->getSpriteFrame();
            if (!frame)
            {
                char basename[256];
                sprintf(basename, "KL_B_Skill2_%04d.png", i);
                frame = SpriteFrameCache::getInstance()->getSpriteFrameByName(basename);
            }
            if (frame) frames.pushBack(frame);
            else break;
        }
        if (!frames.empty())
        {
            // 使用与其它攻击动画类似的帧间隔（可按需调整）
            _animBChengWuJie = Animation::createWithSpriteFrames(frames, 0.10f);
            _animBChengWuJie->retain();
        }
    }

    GAME_LOG("%s: animations loaded (AIdle=%s, AChange=%s, BMove=%s, BAtk=%s, BChange=%s)",
        LOG_TAG,
        _animAIdle ? "OK" : "NO",
        _animAChangeToB ? "OK" : "NO",
        _animBMove ? "OK" : "NO",
        _animBAttack ? "OK" : "NO",
        _animBChangeToC ? "OK" : "NO");
}

bool KuiLongBoss::isPoisonable() const
{
    // 在阶段 A 或 转场期间免疫毒（不会收到剧毒特效）
    return !(_phase == PHASE_A || _phase == TRANSITION_A_TO_B);
}

void KuiLongBoss::update(float dt)
{
    Enemy::update(dt);

    // 更新阶段计时器（示例）
    if (_phase == PHASE_A)
    {
        _phaseTimer += dt;
        if (_phaseTimer >= _phaseADuration)
        {
            // 进入转场
            _phase = TRANSITION_A_TO_B;
            _phaseTimer = 0.0f;

            // 移除自身作为 stealth source（转场/后续阶段将可被识别）
            this->removeStealthSource((void*)this);

            // 播放转场动画（如果有）
            if (_animAChangeToB && _sprite)
            {
                // 停止当前 idle 循环（如果在播放）
                _sprite->stopActionByTag(KUI_LONG_MOVE_TAG);
                _moveAnimPlaying = false;

                auto animate = Animate::create(_animAChangeToB);
                animate->setTag(KUI_LONG_CHANGE_TAG);

                // 在动画结束时立即切换到 PHASE_B，使其可以立刻被识别/受伤/攻击/移动
                auto onFinish = CallFunc::create([this]() {
                    // 立即进入 PHASE_B
                    this->_phase = PHASE_B;
                    GAME_LOG("%s: Transition animation finished -> entered PHASE_B", LOG_TAG);

                    // 启动 B 阶段的移动循环动画（如果存在）
                    if (this->_animBMove && this->_sprite)
                    {
                        auto bmoveAnimate = Animate::create(this->_animBMove);
                        auto repeat = RepeatForever::create(bmoveAnimate);
                        repeat->setTag(KUI_LONG_MOVE_TAG);
                        this->_sprite->runAction(repeat);
                        this->_moveAnimPlaying = true;
                    }

                    // 其他进入 B 的初始化（若需）可以在此添加
                    });

                // 使用 Sequence 确保动画结束后立即执行回调
                auto seq = Sequence::create(animate, onFinish, nullptr);
                seq->setTag(KUI_LONG_CHANGE_TAG);
                _sprite->runAction(seq);
            }
            else
            {
                // 没有转场动画，直接进入 PHASE_B
                this->_phase = PHASE_B;
                GAME_LOG("%s: No transition animation -> entered PHASE_B immediately", LOG_TAG);

                // 立即开始 B move 动画（若有）
                if (_animBMove && _sprite)
                {
                    auto bmoveAnimate = Animate::create(_animBMove);
                    auto repeat = RepeatForever::create(bmoveAnimate);
                    repeat->setTag(KUI_LONG_MOVE_TAG);
                    _sprite->runAction(repeat);
                    _moveAnimPlaying = true;
                }
            }
        }
    }

    // 更新血条显示（随位置/HP变化）
    if (_bossHPBar)
    {
        float percent = (getMaxHP() > 0) ? (static_cast<float>(getHP()) / static_cast<float>(getMaxHP()) * 100.0f) : 0.0f;
        _bossHPBar->setPercent(percent);
    }
    if (_bossHPLabel)
    {
        char buf[64];
        sprintf(buf, "%d/%d", getHP(), getMaxHP());
        _bossHPLabel->setString(buf);
    }

    // 更新技能冷却计时
    if (_skillCooldownTimer < _skillCooldown)
    {
        _skillCooldownTimer += dt;
        if (_skillCooldownTimer > _skillCooldown) _skillCooldownTimer = _skillCooldown;
    }
}

void KuiLongBoss::executeAI(Player* player, float dt)
{
    // 简化示例：阶段 A 不做任何攻击/移动，保持隐身
    if (_phase == PHASE_A || _phase == TRANSITION_A_TO_B)
    {
        // 保持 idle
        return;
    }

    // 阶段 B 行为（示例）
    if (_phase == PHASE_B)
    {
        // 优先技能：若靠近玩家、冷却就绪且未在攻击/技能中，则触发 ChengWuJie
        if (player && !this->_skillPlaying && this->canUseChengWuJie())
        {
            float distSqr = (player->getPosition() - this->getPosition()).lengthSquared();
            if (distSqr <= (_skillRange * _skillRange))
            {
                // 触发技能（技能会设置状态与冷却）
                this->startChengWuJie(player);
                // 触发后直接返回（技能不可被打断，优先级高）
                return;
            }
        }

        // 这里写正常的追击/攻击逻辑（当技能未触发）
        Enemy::executeAI(player, dt);
    }
}

void KuiLongBoss::attack()
{
    // 当正在放技能时，禁止被普通攻击流程打断
    if (_skillPlaying) return;

    if (_phase == PHASE_A || _phase == TRANSITION_A_TO_B) return;

    if (!canAttack()) return;

    // 进入攻击状态并重置冷却
    setState(EntityState::ATTACK);
    resetAttackCooldown();

    // 停止移动循环（但不要 stopAllActions，以免影响其他并行动画）
    if (_sprite)
    {
        auto moveAct = _sprite->getActionByTag(KUI_LONG_MOVE_TAG);
        if (moveAct) _sprite->stopAction(moveAct);
        _moveAnimPlaying = false;
    }

    // 取消可能存在的旧的前摇/延迟（防止重复调度）
    this->stopActionByTag(KUI_LONG_WINDUP_TAG);
    if (_sprite)
    {
        // 也取消 sprite 上旧的前摇视觉（但不要播放完整攻击动画这里）
        auto prevWind = _sprite->getActionByTag(KUI_LONG_WINDUP_TAG);
        if (prevWind) _sprite->stopAction(prevWind);
    }

    // 使用 Enemy 提供的 windup 时长来安排“伤害触发时点”并在风挡结束时播放真正的攻击动画。
    // 关键：不要在 windup 完成时立刻把状态设回 IDLE；而是调用 playAttackAnimation()，
    // 由 playAttackAnimation 在动画播放完毕后再将状态重置为 IDLE（保证动画完整播放）。
    float windup = this->getAttackWindup();
    auto delay = DelayTime::create(windup);
    auto startAttackAnim = CallFunc::create([this]() {
        // 伤害触发点通常由 Enemy 层的逻辑处理（在 windup 时间点），
        // 这里确保视觉上播放攻击命中动画并在动画结束后恢复状态。
        this->playAttackAnimation();
    });
    auto seq = Sequence::create(delay, startAttackAnim, nullptr);
    seq->setTag(KUI_LONG_WINDUP_TAG);
    this->runAction(seq);
}

void KuiLongBoss::playAttackAnimation()
{
    // 技能释放期间不允许被普通攻击动画替换
    if (_skillPlaying) return;

    if (_phase == PHASE_A || _phase == TRANSITION_A_TO_B) return;

    if (!_sprite) return;

    // 如果已经死亡则忽略
    if (_currentState == EntityState::DIE) return;

    // 确保处于 ATTACK 状态以阻止移动
    if (_currentState != EntityState::ATTACK)
    {
        setState(EntityState::ATTACK);
    }

    // 停止移动循环动作（若在播放）
    auto moveAct = _sprite->getActionByTag(KUI_LONG_MOVE_TAG);
    if (moveAct) _sprite->stopAction(moveAct);
    _moveAnimPlaying = false;

    // 停止前摇动画（如果还在播放）
    auto wind = _sprite->getActionByTag(KUI_LONG_WINDUP_TAG);
    if (wind) _sprite->stopAction(wind);

    if (_animBAttack)
    {
        // 停止已有的命中动作
        auto prevHit = _sprite->getActionByTag(KUI_LONG_HIT_TAG);
        if (prevHit) _sprite->stopAction(prevHit);

        _animBAttack->setRestoreOriginalFrame(true);
        auto animate = Animate::create(_animBAttack);
        animate->setTag(KUI_LONG_HIT_TAG);

        // 播放完后恢复到移动动画第一帧（初始状态），然后再让 AI/next tick 决定是启动移动循环还是继续攻击
        auto callback = CallFunc::create([this]() {
            // 恢复到 B 阶段移动动画的第一帧（如果存在）
            if (_animBMove && _sprite)
            {
                auto frames = _animBMove->getFrames();
                if (!frames.empty())
                {
                    _sprite->setSpriteFrame(frames.front()->getSpriteFrame());
                }
            }
            // 播放结束后把状态重回 IDLE（前提不是死亡）
            if (_currentState != EntityState::DIE)
            {
                setState(EntityState::IDLE);
            }
            // 注意：不要立即启动移动循环，这样能保证“先回到第一帧初始状态，再由 AI/移动逻辑决定下一步”。
            // 如果下一帧 AI 调用 move(...)（因为需要追击），move() 会检测到非 ATTACK 状态并启动移动循环动画。
        });
        auto seq = Sequence::create(animate, callback, nullptr);
        seq->setTag(KUI_LONG_HIT_TAG);
        _sprite->runAction(seq);
    }
}

void KuiLongBoss::move(const Vec2& direction, float dt)
{
    // 技能或攻击期间禁止移动（技能不可被打断）
    if (_currentState == EntityState::ATTACK || _currentState == EntityState::SKILL || _currentState == EntityState::DIE)
    {
        // 保持位置不变
        Character::move(Vec2::ZERO, dt);
        return;
    }

    // 如果没有移动向量，停止移动动画并保持移动动画第一帧（如果是 B 阶段）
    if (direction.lengthSquared() <= 1.0f)
    {
        // 停止移动动作（如果存在）
        if (_sprite)
        {
            auto act = _sprite->getActionByTag(KUI_LONG_MOVE_TAG);
            if (act)
            {
                _sprite->stopAction(act);
            }

            // 恢复到移动动画的第一帧（若为 B 阶段），否则若在 A 阶段恢复 AIdle 第一帧
            if (_phase == PHASE_B && _animBMove)
            {
                auto frames = _animBMove->getFrames();
                if (!frames.empty())
                {
                    _sprite->setSpriteFrame(frames.front()->getSpriteFrame());
                }
            }
            else if (_phase == PHASE_A && _animAIdle)
            {
                auto frames = _animAIdle->getFrames();
                if (!frames.empty())
                {
                    _sprite->setSpriteFrame(frames.front()->getSpriteFrame());
                }
            }
        }

        Character::move(Vec2::ZERO, dt); // 更新状态为 IDLE
        _moveAnimPlaying = false;
        return;
    }

    // 有移动向量：实际移动实体
    Vec2 dirNorm = direction.getNormalized();
    Character::move(dirNorm, dt);

    // 设置翻转（左右朝向）
    if (_sprite)
    {
        _sprite->setFlippedX(dirNorm.x < 0.0f);
    }

    // 启动 B 阶段的移动循环动画（如果在 B 阶段并且还未在播放）
    if (_phase == PHASE_B && _animBMove && _sprite)
    {
        if (!_sprite->getActionByTag(KUI_LONG_MOVE_TAG))
        {
            auto animate = Animate::create(_animBMove);
            auto repeat = RepeatForever::create(animate);
            repeat->setTag(KUI_LONG_MOVE_TAG);
            _sprite->runAction(repeat);
            _moveAnimPlaying = true;
        }
    }
}

void KuiLongBoss::takeDamage(int damage)
{
    // 在阶段 A 或 转场期间完全免疫所有伤害
    if (_phase == PHASE_A || _phase == TRANSITION_A_TO_B)
    {
        GAME_LOG("%s: damage ignored in PhaseA/Transition (incoming=%d)", LOG_TAG, damage);
        return;
    }

    // 如果正在释放技能，不改变技能流程（仍允许扣血，但不应触发打断逻辑）
    if (_skillPlaying)
    {
        // 直接调用基类处理生命值/护甲等，但尽量避免触发中断动画/状态（假定基类不会强制 stop skill）
        Enemy::takeDamage(damage);
        return;
    }

    // 否则调用基类实现（包含 Cup 分担逻辑等）
    Enemy::takeDamage(damage);
}

int KuiLongBoss::takeDamageReported(int damage)
{
    // 在阶段 A 或 转场期间完全免疫所有伤害，返回实际生效值 0
    if (_phase == PHASE_A || _phase == TRANSITION_A_TO_B)
    {
        GAME_LOG("%s: takeDamageReported ignored in PhaseA/Transition (incoming=%d)", LOG_TAG, damage);
        return 0;
    }

    // 如果正在释放技能，仍然返回基类计算的实际生效值，但不应触发打断
    if (_skillPlaying)
    {
        return Enemy::takeDamageReported(damage);
    }

    // 否则调用基类并返回实际生效值
    return Enemy::takeDamageReported(damage);
}

void KuiLongBoss::die()
{
    // 如果已经进入 DIE 状态则忽略重复调用
    if (_currentState == EntityState::DIE) return;

    // 标记为死亡状态并停止移动/攻击相关动作
    setState(EntityState::DIE);

    // 先移除 stealth source（避免悬空指针）
    this->removeStealthSource((void*)this);

    // 停止 sprite 上的所有行为相关动作，确保转场动画能正确播放
    if (_sprite)
    {
        _sprite->stopActionByTag(KUI_LONG_MOVE_TAG);
        _sprite->stopActionByTag(KUI_LONG_WINDUP_TAG);
        _sprite->stopActionByTag(KUI_LONG_HIT_TAG);
        // 也取消可能在播放的转场动作，避免重复
        _sprite->stopActionByTag(KUI_LONG_CHANGE_TAG);
    }

    // 如果处于 B 阶段且存在 B->C 转场动画，则按“第5帧停顿2秒再继续”播放
    if (_phase == PHASE_B && _animBChangeToC && _sprite)
    {
        // 获取原动画帧和单帧间隔
        auto origFrames = _animBChangeToC->getFrames();
        float delayPerUnit = 0.1f;
        // 尝试读取原动画的 delayPerUnit（兼容老版本）
        // Animation::getDelayPerUnit() 在大多数 cocos2d-x 版本可用
        // 若不可用则使用默认 0.1f（创建时也是 0.10f）
#ifdef CC_ANIMATION_GET_DELAY_PER_UNIT
        delayPerUnit = _animBChangeToC->getDelayPerUnit();
#else
        // 如果没有宏支持，尝试直接调用（若不存在此符号会在编译时报错；通常可用）
        delayPerUnit = _animBChangeToC->getDelayPerUnit();
#endif

        const ssize_t total = static_cast<ssize_t>(origFrames.size());
        const ssize_t splitIndex = 5; // 在第5帧处停下（索引为0..4为前5帧）
        if (total >= splitIndex + 1)
        {
            // 构造前半部分（前5帧）
            Vector<SpriteFrame*> firstFrames;
            for (ssize_t i = 0; i < splitIndex && i < total; ++i)
            {
                firstFrames.pushBack(origFrames.at(i)->getSpriteFrame());
            }
            // 构造后半部分（剩余帧）
            Vector<SpriteFrame*> secondFrames;
            for (ssize_t i = splitIndex; i < total; ++i)
            {
                secondFrames.pushBack(origFrames.at(i)->getSpriteFrame());
            }

            // 创建两段 Animation（使用与原动画相同的帧间隔）
            auto anim1 = Animation::createWithSpriteFrames(firstFrames, delayPerUnit);
            auto anim2 = Animation::createWithSpriteFrames(secondFrames, delayPerUnit);
            if (anim1) anim1->setRestoreOriginalFrame(false);
            if (anim2) anim2->setRestoreOriginalFrame(false);

            // 对应的 Animate
            auto animate1 = anim1 ? Animate::create(anim1) : nullptr;
            auto animate2 = anim2 ? Animate::create(anim2) : nullptr;

            // 回调：播放完成后调用基类 die 并移除节点
            auto onFinish = CallFunc::create([this]() {
                Enemy::die();
                this->removeFromParentAndCleanup(true);
            });

            // 组合：前半段 -> 停顿2秒 -> 后半段 -> onFinish
            Vector<FiniteTimeAction*> seqActs;
            if (animate1) seqActs.pushBack(animate1);
            seqActs.pushBack(DelayTime::create(2.0f));
            if (animate2) seqActs.pushBack(animate2);
            seqActs.pushBack(onFinish);

            // 将 Vector 转为 Sequence 参数
            Vector<FiniteTimeAction*> actions = seqActs;
            // Build sequence
            Vector<FiniteTimeAction*> tmp;
            // Create a Sequence through initializer list
            // Since Sequence::create expects variadic, build via creating array is inconvenient;
            // Use helper: chain them manually
            ActionInterval* first = nullptr;
            CallFunc* lastCallback = nullptr;
            // We'll build sequence progressively
            FiniteTimeAction* current = nullptr;
            if (!actions.empty())
            {
                current = actions.at(0);
                for (ssize_t i = 1; i < static_cast<ssize_t>(actions.size()); ++i)
                {
                    current = Sequence::create(static_cast<ActionInterval*>(current), static_cast<FiniteTimeAction*>(actions.at(i)), nullptr);
                }
            }

            // 如果 current 是有效 sequence / action，则运行并设置 tag
            if (current)
            {
                auto seq = dynamic_cast<Action*>(current);
                if (seq)
                {
                    seq->setTag(KUI_LONG_CHANGE_TAG);
                    _sprite->runAction(seq);
                    return;
                }
            }

            // 兜底：若上面构建失败，直接播放整体动画
        }

        // 如果帧数不足或拆分失败，回退到播放整体动画
        _animBChangeToC->setRestoreOriginalFrame(false);
        auto fullAnimate = Animate::create(_animBChangeToC);
        fullAnimate->setTag(KUI_LONG_CHANGE_TAG);
        auto onFinishFallback = CallFunc::create([this]() {
            Enemy::die();
            this->removeFromParentAndCleanup(true);
        });
        auto seqFallback = Sequence::create(fullAnimate, onFinishFallback, nullptr);
        seqFallback->setTag(KUI_LONG_CHANGE_TAG);
        _sprite->runAction(seqFallback);
    }
    else
    {
        // 否则直接立即调用基类死亡处理并移除
        Enemy::die();
        this->removeFromParentAndCleanup(true);
    }
}

// 新方法实现：技能相关
bool KuiLongBoss::canUseChengWuJie() const
{
    return (_skillCooldownTimer >= _skillCooldown) && !_skillPlaying && (_phase == PHASE_B);
}

void KuiLongBoss::resetChengWuJieCooldown()
{
    _skillCooldownTimer = 0.0f;
}

// 启动技能（不可被打断）：播放技能动画、按时序触发 5 次伤害（0, 0.5, 1.0, 1.5, 2.5 秒），动画播放完毕后恢复状态
void KuiLongBoss::startChengWuJie(Player* target)
{
    if (_phase != PHASE_B) return;
    if (!target) return;
    if (!_sprite) return;
    if (!canUseChengWuJie()) return;

    // 标记技能正在播放并设置状态阻止移动/攻击
    _skillPlaying = true;
    setState(EntityState::SKILL); // 使用 SKILL 状态阻止移动/攻击，并区分普通 ATTACK
    resetChengWuJieCooldown();

    // 停止移动循环动画
    auto moveAct = _sprite->getActionByTag(KUI_LONG_MOVE_TAG);
    if (moveAct) _sprite->stopAction(moveAct);
    _moveAnimPlaying = false;

    // 停止任何前摇/命中/转场动作（避免冲突）
    auto wind = _sprite->getActionByTag(KUI_LONG_WINDUP_TAG);
    if (wind) _sprite->stopAction(wind);
    auto prevHit = _sprite->getActionByTag(KUI_LONG_HIT_TAG);
    if (prevHit) _sprite->stopAction(prevHit);
    _sprite->stopActionByTag(KUI_LONG_CHANGE_TAG);

    // 播放技能动画（sprite 层面）
    if (_animBChengWuJie)
    {
        _animBChengWuJie->setRestoreOriginalFrame(true);
        auto animate = Animate::create(_animBChengWuJie);
        animate->setTag(KUI_LONG_SKILL_TAG);

        // 在技能动画结束时恢复状态并清理标志（确保动画播放完毕才恢复）
        auto onSkillFinish = CallFunc::create([this]() {
            // 动画播完，解除技能占用
            _skillPlaying = false;

            // 如果未死亡，恢复为 IDLE（移动会由下一帧 AI/ move 决定是否启动）
            if (_currentState != EntityState::DIE)
            {
                setState(EntityState::IDLE);
            }
        });

        auto seq = Sequence::create(animate, onSkillFinish, nullptr);
        seq->setTag(KUI_LONG_SKILL_TAG);
        _sprite->runAction(seq);
    }
    else
    {
        // 若没有技能动画，也要在技能结束后恢复状态（这里用保守长度）
        auto fallbackDelay = DelayTime::create(2.5f);
        auto onFinish = CallFunc::create([this]() {
            _skillPlaying = false;
            if (_currentState != EntityState::DIE) setState(EntityState::IDLE);
        });
        auto seq = Sequence::create(fallbackDelay, onFinish, nullptr);
        seq->setTag(KUI_LONG_SKILL_TAG);
        this->runAction(seq);
    }

    // 在节点上安排 5 次伤害调度（受技能动画与技能状态保护）
    auto damageSeq = Sequence::create(
        DelayTime::create(0.0f), CallFunc::create([this, target]() {
            if (!target) return;
            if (_currentState == EntityState::DIE) return;
            float distSqr = (target->getPosition() - this->getPosition()).lengthSquared();
            if (distSqr <= (_skillRange * _skillRange)) target->takeDamage(_skillDamagePerHit);
        }),
        DelayTime::create(0.5f), CallFunc::create([this, target]() {
            if (!target) return;
            if (_currentState == EntityState::DIE) return;
            float distSqr = (target->getPosition() - this->getPosition()).lengthSquared();
            if (distSqr <= (_skillRange * _skillRange)) target->takeDamage(_skillDamagePerHit);
        }),
        DelayTime::create(0.5f), CallFunc::create([this, target]() {
            if (!target) return;
            if (_currentState == EntityState::DIE) return;
            float distSqr = (target->getPosition() - this->getPosition()).lengthSquared();
            if (distSqr <= (_skillRange * _skillRange)) target->takeDamage(_skillDamagePerHit);
        }),
        DelayTime::create(0.5f), CallFunc::create([this, target]() {
            if (!target) return;
            if (_currentState == EntityState::DIE) return;
            float distSqr = (target->getPosition() - this->getPosition()).lengthSquared();
            if (distSqr <= (_skillRange * _skillRange)) target->takeDamage(_skillDamagePerHit);
        }),
        DelayTime::create(1.0f), CallFunc::create([this, target]() {
            if (!target) return;
            if (_currentState == EntityState::DIE) return;
            float distSqr = (target->getPosition() - this->getPosition()).lengthSquared();
            if (distSqr <= (_skillRange * _skillRange)) target->takeDamage(_skillDamagePerHit);
        }),
        nullptr);

    // 给该动作设置 tag，避免重复调度或误触发
    damageSeq->setTag(KUI_LONG_SKILL_DAMAGE_TAG);
    this->runAction(damageSeq);
}