#include "KuiLongBoss.h"
#include "cocos2d.h"
#include "Entities/Player/Player.h"
#include "ui/CocosGUI.h"

USING_NS_CC;

static const char* LOG_TAG = "KuiLongBoss";

KuiLongBoss::KuiLongBoss()
    : _phase(PHASE_A)
    , _phaseTimer(0.0f)
    , _phaseADuration(30.0f)
    , _animAIdle(nullptr)
    , _animAChangeToB(nullptr)
    , _animBMove(nullptr)
    , _animBAttack(nullptr)
    , _animBChangeToC(nullptr)
    , _moveAnimPlaying(false)
    , _bossHPBar(nullptr)
    , _bossHPLabel(nullptr)
    , _bossBarOffsetY(12.0f)
{
}

KuiLongBoss::~KuiLongBoss()
{
    if (_animAIdle) { _animAIdle->release(); _animAIdle = nullptr; }
    if (_animAChangeToB) { _animAChangeToB->release(); _animAChangeToB = nullptr; }
    if (_animBMove) { _animBMove->release(); _animBMove = nullptr; }
    if (_animBAttack) { _animBAttack->release(); _animBAttack = nullptr; }
    if (_animBChangeToC) { _animBChangeToC->release(); _animBChangeToC = nullptr; }

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
    setSightRange(300.0f);
    setAttackRange(60.0f);

    // 攻击间隔：2 秒（Boss 的攻击冷却），通过 Character::setAttackCooldown / getAttackCooldown 管理
    setAttackCooldown(2.0f);

    // 阶段初始
    _phase = PHASE_A;
    _phaseTimer = 0.0f;
    _phaseADuration = 30.0f;

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
            if (!frame)
            {
                char basename[256];
                sprintf(basename, "KL_B_Attack_%04d.png", i);
                frame = SpriteFrameCache::getInstance()->getSpriteFrameByName(basename);
            }
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
        // 这里写正常的追击/攻击逻辑
        Enemy::executeAI(player, dt);
    }
}

void KuiLongBoss::attack()
{
    if (_phase == PHASE_A || _phase == TRANSITION_A_TO_B) return;

    if (!canAttack()) return;

    // 进入攻击状态并重置冷却
    setState(EntityState::ATTACK);
    resetAttackCooldown();

    // 播放攻击前摇：只停止移动循环动作，避免 stopAllActions 干扰其它行为
    if (_animBAttack && _sprite)
    {
        // 停止移动循环（但不 stopAllActions）
        auto moveAct = _sprite->getActionByTag(KUI_LONG_MOVE_TAG);
        if (moveAct) _sprite->stopAction(moveAct);

        // 停止可能存在的前摇动作
        auto prevWind = _sprite->getActionByTag(KUI_LONG_WINDUP_TAG);
        if (prevWind) _sprite->stopAction(prevWind);

        // 作为前摇在 sprite 上播放（视觉），但实际伤害由 Enemy::executeAI 的 windup 回调触发
        _animBAttack->setRestoreOriginalFrame(true);
        auto animate = Animate::create(_animBAttack);
        animate->setTag(KUI_LONG_WINDUP_TAG);
        _sprite->runAction(animate);

        // 使用 Enemy 提供的 windup 时长来在节点上安排状态回退（与 Ayao 保持一致）
        float windup = this->getAttackWindup();
        auto delay = DelayTime::create(windup);
        auto callback = CallFunc::create([this]() {
            if (_currentState == EntityState::ATTACK)
            {
                setState(EntityState::IDLE);
            }
        });
        auto seq = Sequence::create(delay, callback, nullptr);
        this->runAction(seq); // 放在节点上，避免和 sprite 上动画冲突
    }
    else
    {
        // 没有攻击动画时仍使用 windup 时长保证同步与反馈
        float windup = this->getAttackWindup();
        auto delay = DelayTime::create(windup);
        auto callback = CallFunc::create([this]() {
            if (_currentState == EntityState::ATTACK)
            {
                setState(EntityState::IDLE);
            }
        });
        this->runAction(Sequence::create(delay, callback, nullptr));
    }
}

void KuiLongBoss::playAttackAnimation()
{
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

        // 播放完后恢复到移动动画第一帧，并把状态设回 IDLE（如果未死亡）
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
            // 恢复状态（前提不是死亡）
            if (_currentState != EntityState::DIE)
            {
                setState(EntityState::IDLE);
            }
        });
        auto seq = Sequence::create(animate, callback, nullptr);
        seq->setTag(KUI_LONG_HIT_TAG);
        _sprite->runAction(seq);
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

    // 否则调用基类并返回实际生效值
    return Enemy::takeDamageReported(damage);
}

void KuiLongBoss::die()
{
    // Boss 特殊死亡逻辑（播放动画/生成奖励等）
    Enemy::die();

    // 确保移除 stealth source（防止悬空指针）
    this->removeStealthSource((void*)this);

    // 其他清理...
}