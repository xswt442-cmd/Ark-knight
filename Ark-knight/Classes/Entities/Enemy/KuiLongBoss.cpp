#include "KuiLongBoss.h"
#include "cocos2d.h"
#include "Entities/Player/Player.h"
#include "ui/CocosGUI.h"
#include "Entities/Enemy/NiLuFire.h" // 新增 include
#include "Scenes/GameScene.h"       // 为了调用 addEnemy / getPlayer
#include "UI/FloatingText.h"        // 新增：显示伤害浮字

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
    , _skillCooldown(10.0f)
    , _skillCooldownTimer(_skillCooldown)
    , _skillRange(220.0f)
    , _skillDamagePerHit(getAttack() * 4)
    , _skillPlaying(false)
    , _skillDamageScheduled(false) // <- 新增初始化
    , _roomBounds(Rect::ZERO) // 新增
    , _niluSpawnTimer(0.0f)
    , _niluSpawnInterval(10.0f) // 每10秒尝试生成
    , _niluSpawnPerInterval(2)
    , _niluMaxPerRoom(12)
    , _niluMinDistance(50.0f)
{
}

void KuiLongBoss::setRoomBounds(const Rect& bounds)
{
    _roomBounds = bounds;
}

// 在文件顶部（include 之后，namespace 使用处之后），插入如下静态辅助函数（或放在匿名命名空间内）
static void collectNiLuFiresRecursive(cocos2d::Node* node, std::vector<class NiLuFire*>& out)
{
    if (!node) return;
    // 检查当前节点是否为 NiLuFire
    NiLuFire* fire = dynamic_cast<NiLuFire*>(node);
    if (fire)
    {
        out.push_back(fire);
    }
    // 递归其子节点
    const auto& children = node->getChildren();
    for (auto child : children)
    {
        collectNiLuFiresRecursive(child, out);
    }
}

// 计算当前房间内合法的 NiLuFire 数量（遍历场景中的 ENEMY tag 并 dynamic_cast）
int KuiLongBoss::countNiLuFiresInRoom() const
{
    Scene* running = Director::getInstance()->getRunningScene();
    if (!running) return 0;

    std::vector<NiLuFire*> found;
    collectNiLuFiresRecursive(running, found);

    int count = 0;
    for (auto fire : found)
    {
        if (!fire) continue;
        // 不再根据 isDead() 跳过：NiLu 初始 HP 为 0，但只要该节点存在应被视作生效
        // 仅统计在该 Boss 房间范围内的 NiLu
        if (!_roomBounds.equals(Rect::ZERO))
        {
            if (!_roomBounds.containsPoint(fire->getPosition())) continue;
        }
        else
        {
            // 若未设置 room bounds，按距离保守计数（兼容旧逻辑）
            if (fire->getPosition().distance(this->getPosition()) >= 2000.0f) continue;
        }
        ++count;
    }
    return count;
}

bool KuiLongBoss::isPositionValidForNiLu(const Vec2& pos) const
{
    // 必须在房间 walkable area 内（留 1 像素边距）
    if (_roomBounds.equals(Rect::ZERO)) return false;
    if (!(pos.x >= _roomBounds.getMinX() + 1.0f && pos.x <= _roomBounds.getMaxX() - 1.0f &&
        pos.y >= _roomBounds.getMinY() + 1.0f && pos.y <= _roomBounds.getMaxY() - 1.0f)) return false;

    // 确保与其他 NiLuFire 距离至少 _niluMinDistance
    Scene* running = Director::getInstance()->getRunningScene();
    if (!running) return false;

    std::vector<NiLuFire*> found;
    collectNiLuFiresRecursive(running, found);

    for (auto fire : found)
    {
        if (!fire) continue;
        if (fire->isDead()) continue;
        // 只考虑同房间内的 NiLu（若 boss 有 bounds）
        if (!_roomBounds.equals(Rect::ZERO))
        {
            if (!_roomBounds.containsPoint(fire->getPosition())) continue;
        }
        if (fire->getPosition().distance(pos) < _niluMinDistance) return false;
    }
    return true;
}

void KuiLongBoss::trySpawnNiLuFires()
{
    // 计数限制
    int existing = countNiLuFiresInRoom();
    if (existing >= _niluMaxPerRoom) return;

    // 每次尝试生成 _niluSpawnPerInterval 个，直到达上限
    int toSpawn = _niluSpawnPerInterval;
    if (existing + toSpawn > _niluMaxPerRoom)
        toSpawn = _niluMaxPerRoom - existing;
    if (toSpawn <= 0) return;

    Scene* running = Director::getInstance()->getRunningScene();
    if (!running) return;
    GameScene* gs = dynamic_cast<GameScene*>(running);
    if (!gs) return;

    // 随机位置采样（简单重试法）
    int attempts = 0;
    int spawned = 0;
    while (spawned < toSpawn && attempts < 50)
    {
        attempts++;
        float rx = CCRANDOM_0_1() * (_roomBounds.getMaxX() - _roomBounds.getMinX()) + _roomBounds.getMinX();
        float ry = CCRANDOM_0_1() * (_roomBounds.getMaxY() - _roomBounds.getMinY()) + _roomBounds.getMinY();
        Vec2 cand(rx, ry);

        if (!isPositionValidForNiLu(cand)) continue;

        // 创建 NiLuFire
        auto fire = NiLuFire::create();
        if (!fire) continue;
        fire->setPosition(cand);

        // 将 NiLu 的攻击力与 KuiLong 保持一致
        fire->setAttack(this->getAttack()); // 也保留 base attack
        // 也给内部字段（NiLuFire 有 _attackDamage 字段用于 performAttackImmediate）
        // 通过 dynamic cast 成功后直接访问私有不是可行；使用 public setAttack 足以（performAttackImmediate 接受 damage 参数）
        // 注册到场景（GameScene::addEnemy 会调用 setRoomBounds）
        gs->addEnemy(fire);

        spawned++;
    }

    if (spawned > 0)
    {
        GAME_LOG("%s: spawned %d NiLuFires (existing before=%d)", LOG_TAG, spawned, existing);
    }
}

bool KuiLongBoss::init()
{
    if (!Enemy::init()) return false;

    // 基础属性（可根据需求调整）
    setEnemyType(EnemyType::MELEE);
    setMaxHP(20000);
    setHP(getMaxHP());
    setMoveSpeed(30.0f); // 原 60.0f -> 30.0f，已降低移速
    setAttack(1800);
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

    // 计时生成 NiLuFire
    if (_currentState != EntityState::DIE)
    {
        _niluSpawnTimer += dt;
        if (_niluSpawnTimer >= _niluSpawnInterval)
        {
            _niluSpawnTimer = 0.0f;
            trySpawnNiLuFires();
        }
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

    // 使用 Enemy 提供的 windup 時長來安排“傷害触发時點”並在風擋結束时同时：
    // 1) 对玩家造成一次普通攻击伤害（若在范围内）
    // 2) 触发房间内所有 NiLuFire 的 performAttackImmediate（伤害 = KuiLong 的攻击力）
    // 之后再播放攻击动画（视觉）
    float windup = this->getAttackWindup();
    auto delay = DelayTime::create(windup);
    auto startAttackAnim = CallFunc::create([this]() {
        // 1) 伤害判定（world-space 判断与 Player）
        Scene* running = Director::getInstance()->getRunningScene();
        if (running)
        {
            GameScene* gs = dynamic_cast<GameScene*>(running);
            if (gs)
            {
                Player* player = gs->getPlayer();
                if (player && player->getParent() && !player->isDead())
                {
                    float distSqr = (player->getPosition() - this->getPosition()).lengthSquared();
                    if (distSqr <= (this->getAttackRange() * this->getAttackRange()))
                    {
                        int dmg = this->getAttack();
                        player->takeDamage(dmg);
                        // 显示红色浮字（Player::takeDamage 中也会显示红色，保留以便一致）
                        Node* parentForText = player->getParent() ? player->getParent() : running;
                        if (parentForText)
                        {
                            FloatingText::show(parentForText, player->getPosition(), std::to_string(dmg), Color3B(220,20,20));
                        }
                    }
                }
            }

            // 2) 同步触发房间内的 NiLuFire 攻击（与奎隆造成伤害时机一致）
            std::vector<NiLuFire*> found;
            collectNiLuFiresRecursive(running, found);
            for (auto fire : found)
            {
                if (!fire) continue;
                // 不用 isDead() 过滤：只按房间范围过滤（NiLu 初始 HP 为 0 但应视为有效）
                if (!_roomBounds.equals(Rect::ZERO))
                {
                    if (!_roomBounds.containsPoint(fire->getPosition())) continue;
                }
                // 传入 KuiLong 的攻击力，强制触发（NiLu 内会在保护期判断：我们在 NiLu 的实现里允许 damage>0 强制触发）
                fire->performAttackImmediate(this->getAttack());
            }
        }

        // 视觉：播放奎隆的攻击动画（在攻击判定后）
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

        // 播放攻击动画并在回调时恢复状态
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

            // 普通攻击触发时，让附近的 NiLuFire 同步释放一次攻击（伤害等于 KuiLong 的攻击力）
            Scene* running = Director::getInstance()->getRunningScene();
            if (running)
            {
                std::vector<NiLuFire*> found;
                collectNiLuFiresRecursive(running, found);
                for (auto fire : found)
                {
                    if (!fire) continue;
                    // 不再按 isDead() 跳过：NiLu 初始 HP 为 0，但存在时应被视作有效
                    // 只触发在同一房间内的 NiLu
                    if (!_roomBounds.equals(Rect::ZERO))
                    {
                        if (!_roomBounds.containsPoint(fire->getPosition())) continue;
                    }
                    fire->performAttackImmediate(this->getAttack());
                }
            }

            // 播放结束后把状态重回 IDLE（前提不是死亡）
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

void KuiLongBoss::startChengWuJie(Player* target)
{
    if (_phase != PHASE_B) return;
    if (!target) return;
    if (!_sprite) return;
    if (!canUseChengWuJie()) return;

    // 标记技能正在播放并设置状态阻止移动/攻击
    _skillPlaying = true;
    setState(EntityState::SKILL);
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
    this->stopActionByTag(KUI_LONG_WINDUP_TAG);
    _sprite->stopActionByTag(KUI_LONG_CHANGE_TAG);

    // 播放技能动画（sprite 层面）
    if (_animBChengWuJie)
    {
        _animBChengWuJie->setRestoreOriginalFrame(true);
        auto animate = Animate::create(_animBChengWuJie);
        animate->setTag(KUI_LONG_SKILL_TAG);

        // 【关键修复】移除此处的回调状态重置。
        // 动画时长(2.3s)短于伤害序列(2.5s)，原逻辑会导致过早重置状态，
        // 从而在技能未结束时插入普通攻击。现在状态重置由伤害序列统一管理。
        _sprite->runAction(animate);
    }
    else
    {
        // 无动画时的 fallback，仅用于占位，不负责重置状态
    }

    // 取消旧的技能伤害序列，防止叠加
    this->stopActionByTag(KUI_LONG_SKILL_DAMAGE_TAG);
    this->_skillDamageScheduled = false;

    // 生成 5 次伤害的动作（时点：0, 0.5, 1.0, 1.5, 2.5）
    int dmgPerHit = this->_skillDamagePerHit;

    Vector<FiniteTimeAction*> acts;
    
    // 辅助 lambda：生成单次伤害回调
    auto createHitFunc = [this, target, dmgPerHit]() {
        return CallFunc::create([this, target, dmgPerHit]() {
            if (!target) return;
            if (this->_currentState == EntityState::DIE) return;
            float distSqr = (target->getPosition() - this->getPosition()).lengthSquared();
            if (distSqr <= (this->_skillRange * this->_skillRange))
            {
                target->takeDamage(dmgPerHit);
            }
        });
    };

    // t = 0
    acts.pushBack(createHitFunc());
    
    // t = 0.5
    acts.pushBack(DelayTime::create(0.5f));
    acts.pushBack(createHitFunc());

    // t = 1.0
    acts.pushBack(DelayTime::create(0.5f));
    acts.pushBack(createHitFunc());

    // t = 1.5
    acts.pushBack(DelayTime::create(0.5f));
    acts.pushBack(createHitFunc());

    // t = 2.5 (gap 1.0s)
    acts.pushBack(DelayTime::create(1.0f));
    acts.pushBack(createHitFunc());

    // 【关键修复】序列结束：统一重置状态
    // 确保在所有伤害打完后，才允许 Boss 进行下一个动作
    acts.pushBack(CallFunc::create([this]() {
        this->_skillDamageScheduled = false;
        this->_skillPlaying = false;
        if (this->_currentState != EntityState::DIE)
        {
            this->setState(EntityState::IDLE);
        }
    }));

    // 逐步拼接 Sequence
    FiniteTimeAction* seqHead = nullptr;
    if (!acts.empty())
    {
        seqHead = acts.at(0);
        for (ssize_t i = 1; i < static_cast<ssize_t>(acts.size()); ++i)
        {
            seqHead = Sequence::create(seqHead, acts.at(i), nullptr);
        }
    }

    if (seqHead)
    {
        seqHead->setTag(KUI_LONG_SKILL_DAMAGE_TAG);
        this->_skillDamageScheduled = true;
        this->runAction(dynamic_cast<Action*>(seqHead));
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

    // 当正在释放技能，不改变技能流程（仍允许扣血，但不应触发打断逻辑）
    if (_skillPlaying)
    {
        // 应用 NiLuFire 带来的伤害减免
        int count = countNiLuFiresInRoom();
        float factor = powf(0.85f, static_cast<float>(count));
        int reduced = static_cast<int>(std::floor(static_cast<float>(damage) * factor + 0.0001f));
        Enemy::takeDamage(reduced);
        return;
    }

    // 否则调用基类实现（包含 Cup 分担逻辑等），但先应用 NiLu 减免
    int count = countNiLuFiresInRoom();
    float factor = powf(0.85f, static_cast<float>(count));
    int reduced = static_cast<int>(std::floor(static_cast<float>(damage) * factor + 0.0001f));
    Enemy::takeDamage(reduced);
}

int KuiLongBoss::takeDamageReported(int damage)
{
    // 在阶段 A 或 转场期间完全免疫所有伤害，返回实际生效值 0
    if (_phase == PHASE_A || _phase == TRANSITION_A_TO_B)
    {
        GAME_LOG("%s: takeDamageReported ignored in PhaseA/Transition (incoming=%d)", LOG_TAG, damage);
        return 0;
    }

    // 计算 NiLuFire 减免
    int count = countNiLuFiresInRoom();
    float factor = powf(0.85f, static_cast<float>(count));
    int reduced = static_cast<int>(std::floor(static_cast<float>(damage) * factor + 0.0001f));

    // 如果正在放技能，仍然允许扣血但不打断
    if (_skillPlaying)
    {
        return Enemy::takeDamageReported(reduced);
    }

    return Enemy::takeDamageReported(reduced);
}

void KuiLongBoss::die()
{
    // 如果已经进入 DIE 状态则忽略重复调用
    if (_currentState == EntityState::DIE) return;

    // 标记为死亡状态并停止移动/攻击相关动作
    setState(EntityState::DIE);

    // 先移除 stealth source（避免悬空指针）
    this->removeStealthSource((void*)this);

    // 立即移除同房间的所有 NiLuFire（不计入击杀、且不触发自爆/额外效果）
    Scene* running = Director::getInstance()->getRunningScene();
    if (running)
    {
        std::vector<NiLuFire*> found;
        collectNiLuFiresRecursive(running, found);
        for (auto fire : found)
        {
            if (!fire) continue;
            // 只移除在该 Boss 房间范围内的 NiLu（若有 room bounds）
            if (!_roomBounds.equals(Rect::ZERO))
            {
                if (!_roomBounds.containsPoint(fire->getPosition())) continue;
            }
            // 停止其所有动作与调度，直接移除，避免触发自爆等副作用
            fire->stopAllActions();
            fire->unscheduleAllCallbacks();
            // 若需要更安全，也可先设置状态为 DIE
            fire->setState(EntityState::DIE);
            fire->removeFromParentAndCleanup(true);
        }
    }

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
        // （保持你原有的转场播放逻辑不变）
        // 获取原动画帧和单帧间隔
        auto origFrames = _animBChangeToC->getFrames();
        float delayPerUnit = 0.1f;
#ifdef CC_ANIMATION_GET_DELAY_PER_UNIT
        delayPerUnit = _animBChangeToC->getDelayPerUnit();
#else
        delayPerUnit = _animBChangeToC->getDelayPerUnit();
#endif

        const ssize_t total = static_cast<ssize_t>(origFrames.size());
        const ssize_t splitIndex = 5; // 在第5帧处停下（索引为0..4为前5帧）
        if (total >= splitIndex + 1)
        {
            Vector<SpriteFrame*> firstFrames;
            for (ssize_t i = 0; i < splitIndex && i < total; ++i)
            {
                firstFrames.pushBack(origFrames.at(i)->getSpriteFrame());
            }
            Vector<SpriteFrame*> secondFrames;
            for (ssize_t i = splitIndex; i < total; ++i)
            {
                secondFrames.pushBack(origFrames.at(i)->getSpriteFrame());
            }

            auto anim1 = Animation::createWithSpriteFrames(firstFrames, delayPerUnit);
            auto anim2 = Animation::createWithSpriteFrames(secondFrames, delayPerUnit);
            if (anim1) anim1->setRestoreOriginalFrame(false);
            if (anim2) anim2->setRestoreOriginalFrame(false);

            auto animate1 = anim1 ? Animate::create(anim1) : nullptr;
            auto animate2 = anim2 ? Animate::create(anim2) : nullptr;

            auto onFinish = CallFunc::create([this]() {
                Enemy::die();
                this->removeFromParentAndCleanup(true);
            });

            Vector<FiniteTimeAction*> seqActs;
            if (animate1) seqActs.pushBack(animate1);
            seqActs.pushBack(DelayTime::create(2.0f));
            if (animate2) seqActs.pushBack(animate2);
            seqActs.pushBack(onFinish);

            FiniteTimeAction* current = nullptr;
            if (!seqActs.empty())
            {
                current = seqActs.at(0);
                for (ssize_t i = 1; i < static_cast<ssize_t>(seqActs.size()); ++i)
                {
                    current = Sequence::create(static_cast<ActionInterval*>(current), static_cast<FiniteTimeAction*>(seqActs.at(i)), nullptr);
                }
            }

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
        }

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

void KuiLongBoss::move(const cocos2d::Vec2& direction, float dt)
{
    // 阶段/技能/攻击期间禁止移动
    if (_currentState == EntityState::DIE || _skillPlaying || _currentState == EntityState::ATTACK) return;

    // 若没有实际方向，则停止移动动画
    if (direction.equals(cocos2d::Vec2::ZERO))
    {
        if (_moveAnimPlaying && _sprite)
        {
            auto act = _sprite->getActionByTag(KUI_LONG_MOVE_TAG);
            if (act) _sprite->stopAction(act);
            _moveAnimPlaying = false;
        }
        return;
    }

    // 若尚未播放移动动画，根据当前阶段尝试播放合适的循环移动动画
    if (_sprite && !_moveAnimPlaying)
    {
        cocos2d::Animation* chosen = nullptr;
        // 在 B/C 阶段优先使用 B move，否则使用 A idle（尽量保证有动画）
        if ((_phase == PHASE_B || _phase == PHASE_C) && _animBMove) chosen = _animBMove;
        else if (_animAIdle) chosen = _animAIdle;

        if (chosen)
        {
            auto animate = cocos2d::Animate::create(chosen);
            auto repeat = cocos2d::RepeatForever::create(animate);
            repeat->setTag(KUI_LONG_MOVE_TAG);
            _sprite->runAction(repeat);
            _moveAnimPlaying = true;
        }
    }

    // 简单位置更新（按 direction * dt 平移，避免依赖可能缺失的 getter）
    this->setPosition(this->getPosition() + direction * dt);
}

KuiLongBoss::~KuiLongBoss()
{
    // 停止所有调度/动作，避免悬挂回调
    this->stopAllActions();
    if (_sprite) _sprite->stopAllActions();

    // 移除 stealth source（防止悬空指针）
    this->removeStealthSource((void*)this);

    // 释放 retain 的 Animation（兼容 Cocos2d-x retain/release 使用）
    if (_animAIdle)        { _animAIdle->release();        _animAIdle = nullptr; }
    if (_animAChangeToB)   { _animAChangeToB->release();   _animAChangeToB = nullptr; }
    if (_animBMove)        { _animBMove->release();        _animBMove = nullptr; }
    if (_animBAttack)      { _animBAttack->release();      _animBAttack = nullptr; }
    if (_animBChangeToC)   { _animBChangeToC->release();   _animBChangeToC = nullptr; }
    if (_animBChengWuJie)  { _animBChengWuJie->release();  _animBChengWuJie = nullptr; }

    // 清理 UI / 精灵引用
    if (_bossHPBar)    { _bossHPBar->removeFromParentAndCleanup(true); _bossHPBar = nullptr; }
    if (_bossHPLabel)  { _bossHPLabel->removeFromParentAndCleanup(true); _bossHPLabel = nullptr; }
    if (_skillSprite)  { _skillSprite->removeFromParentAndCleanup(true); _skillSprite = nullptr; }
}

// 新增实现：通用逐帧加载、技能可用判断与重置冷却

cocos2d::Animation* KuiLongBoss::loadAnimationFrames(const std::string& folder, const std::string& prefix, int maxFrames, float delayPerUnit)
{
    Vector<SpriteFrame*> frames;
    for (int i = 1; i <= maxFrames; ++i)
    {
        char filename[512];
        sprintf(filename, "%s/%s_%04d.png", folder.c_str(), prefix.c_str(), i);
        SpriteFrame* frame = nullptr;
        auto s = Sprite::create(filename);
        if (s) frame = s->getSpriteFrame();
        if (!frame)
        {
            char basename[256];
            sprintf(basename, "%s_%04d.png", prefix.c_str(), i);
            frame = SpriteFrameCache::getInstance()->getSpriteFrameByName(basename);
        }
        if (frame) frames.pushBack(frame);
        else break;
    }

    if (frames.empty()) return nullptr;

    auto anim = Animation::createWithSpriteFrames(frames, delayPerUnit);
    return anim; // 返回 autoreleased，调用方可按需 retain()
}

bool KuiLongBoss::canUseChengWuJie() const
{
    // 仅在 PHASE_B 且未在技能/死亡状态且冷却已到时可用
    if (_phase != PHASE_B) return false;
    if (_skillPlaying) return false;
    if (_currentState == EntityState::DIE) return false;
    return (_skillCooldownTimer >= _skillCooldown);
}

void KuiLongBoss::resetChengWuJieCooldown()
{
    _skillCooldownTimer = 0.0f;
}