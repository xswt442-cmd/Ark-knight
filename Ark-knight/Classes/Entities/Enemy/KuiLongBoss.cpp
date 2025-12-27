#include "KuiLongBoss.h"
#include "cocos2d.h"
#include "Entities/Player/Player.h"
#include "ui/CocosGUI.h"
#include "Entities/Enemy/NiLuFire.h"
#include "Entities/Enemy/Boat.h" // 新增 include
#include "Scenes/GameScene.h"
#include "UI/FloatingText.h"

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
    , _animBChengWuJie(nullptr)
    , _animCSS_Start(nullptr)
    , _animCSS_Idle(nullptr)
    , _animCSS_End(nullptr)
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
    , _skillDamageScheduled(false)
    , _roomBounds(Rect::ZERO)
    , _niluSpawnTimer(0.0f)
    , _niluSpawnInterval(10.0f)
    , _niluSpawnPerInterval(2)
    , _niluMaxPerRoom(12)
    , _niluMinDistance(50.0f)
    , _threshold75Triggered(false)
    , _threshold50Triggered(false)
    , _threshold25Triggered(false)
    , _chengSanShenTimer(0.0f)
    , _chengSanShenDuration(30.0f)
    , _summonedBoat(nullptr)
{
}

void KuiLongBoss::setRoomBounds(const Rect& bounds)
{
    _roomBounds = bounds;
}

// 辅助函数：递归收集 NiLuFire (保持原样)
static void collectNiLuFiresRecursive(cocos2d::Node* node, std::vector<class NiLuFire*>& out)
{
    if (!node) return;
    NiLuFire* fire = dynamic_cast<NiLuFire*>(node);
    if (fire) out.push_back(fire);
    const auto& children = node->getChildren();
    for (auto child : children) collectNiLuFiresRecursive(child, out);
}

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
        if (!_roomBounds.equals(Rect::ZERO)) {
            if (!_roomBounds.containsPoint(fire->getPosition())) continue;
        } else {
            if (fire->getPosition().distance(this->getPosition()) >= 2000.0f) continue;
        }
        ++count;
    }
    return count;
}

bool KuiLongBoss::isPositionValidForNiLu(const Vec2& pos) const
{
    if (_roomBounds.equals(Rect::ZERO)) return false;
    if (!(pos.x >= _roomBounds.getMinX() + 1.0f && pos.x <= _roomBounds.getMaxX() - 1.0f &&
        pos.y >= _roomBounds.getMinY() + 1.0f && pos.y <= _roomBounds.getMaxY() - 1.0f)) return false;

    Scene* running = Director::getInstance()->getRunningScene();
    if (!running) return false;
    std::vector<NiLuFire*> found;
    collectNiLuFiresRecursive(running, found);
    for (auto fire : found)
    {
        if (!fire) continue;
        if (fire->isDead()) continue;
        if (!_roomBounds.equals(Rect::ZERO)) {
            if (!_roomBounds.containsPoint(fire->getPosition())) continue;
        }
        if (fire->getPosition().distance(pos) < _niluMinDistance) return false;
    }
    return true;
}

void KuiLongBoss::trySpawnNiLuFires()
{
    int existing = countNiLuFiresInRoom();
    if (existing >= _niluMaxPerRoom) return;
    int toSpawn = _niluSpawnPerInterval;
    if (existing + toSpawn > _niluMaxPerRoom) toSpawn = _niluMaxPerRoom - existing;
    if (toSpawn <= 0) return;

    Scene* running = Director::getInstance()->getRunningScene();
    if (!running) return;
    GameScene* gs = dynamic_cast<GameScene*>(running);
    if (!gs) return;

    int attempts = 0;
    int spawned = 0;
    while (spawned < toSpawn && attempts < 50)
    {
        attempts++;
        float rx = CCRANDOM_0_1() * (_roomBounds.getMaxX() - _roomBounds.getMinX()) + _roomBounds.getMinX();
        float ry = CCRANDOM_0_1() * (_roomBounds.getMaxY() - _roomBounds.getMinY()) + _roomBounds.getMinY();
        Vec2 cand(rx, ry);
        if (!isPositionValidForNiLu(cand)) continue;

        auto fire = NiLuFire::create();
        if (!fire) continue;
        fire->setPosition(cand);
        fire->setAttack(this->getAttack());
        gs->addEnemy(fire);
        spawned++;
    }
}

bool KuiLongBoss::init()
{
    if (!Enemy::init()) return false;

    setEnemyType(EnemyType::MELEE);
    setMaxHP(20000);
    setHP(getMaxHP());
    setMoveSpeed(30.0f);
    setAttack(1800);
    setSightRange(1000.0f);
    setAttackRange(120.0f);
    setAttackCooldown(2.0f);
    setAttackWindup(0.1f);

    _phase = PHASE_A;
    _phaseTimer = 0.0f;
    _phaseADuration = 10.0f;

    loadAnimations();

    if (_animAIdle) {
        auto frames = _animAIdle->getFrames();
        if (!frames.empty()) {
            auto sprite = Sprite::createWithSpriteFrame(frames.front()->getSpriteFrame());
            this->bindSprite(sprite);
        }
    } else {
        auto sprite = Sprite::create();
        sprite->setTextureRect(Rect(0, 0, 48, 48));
        sprite->setColor(Color3B::RED);
        this->bindSprite(sprite);
    }

    if (_sprite && _animAIdle) {
        auto animate = Animate::create(_animAIdle);
        auto repeat = RepeatForever::create(animate);
        repeat->setTag(KUI_LONG_MOVE_TAG);
        _sprite->runAction(repeat);
        _moveAnimPlaying = true;
    }

    Vec2 center = Vec2::ZERO;
    Size vs = Director::getInstance()->getVisibleSize();
    center = Vec2(vs.width * 0.5f, vs.height * 0.5f);
    this->setPosition(center);

    this->addStealthSource((void*)this);

    // 血条 UI (保持原样)
    {
        float barWidth = 120.0f;
        float barHeight = 10.0f;
        float bgGlobalZ = static_cast<float>(Constants::ZOrder::FLOOR + 1);
        float fillGlobalZ = static_cast<float>(Constants::ZOrder::FLOOR + 2);
        float labelGlobalZ = static_cast<float>(Constants::ZOrder::FLOOR + 3);

        auto bg = Sprite::create("UIs/StatusBars/Bars/EmplyBar.png");
        if (bg) {
            bg->setAnchorPoint(Vec2(0.5f, 0.5f));
            bg->setScaleX(barWidth / bg->getContentSize().width);
            bg->setScaleY(barHeight / bg->getContentSize().height);
            bg->setPosition(Vec2(0.0f, -(_sprite ? _sprite->getBoundingBox().size.height * 0.5f : 24.0f) - _bossBarOffsetY));
            bg->setGlobalZOrder(bgGlobalZ);
            this->addChild(bg, 0);
        }

        _bossHPBar = cocos2d::ui::LoadingBar::create("UIs/StatusBars/Bars/HealthFill.png");
        if (_bossHPBar) {
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
        if (_bossHPLabel) {
            _bossHPLabel->setAnchorPoint(Vec2(0.5f, 0.5f));
            _bossHPLabel->setTextColor(Color4B::WHITE);
            _bossHPLabel->setPosition(Vec2(0.0f, -(_sprite ? _sprite->getBoundingBox().size.height * 0.5f : 24.0f) - _bossBarOffsetY - 12.0f));
            _bossHPLabel->setGlobalZOrder(labelGlobalZ);
            this->addChild(_bossHPLabel, 2);
        }
    }

    return true;
}

void KuiLongBoss::loadAnimations()
{
    // 使用辅助函数加载原有动画
    _animAIdle = loadAnimationFrames("Enemy/_BOSS_KuiLong/Boss_A_Idle", "KL_A_Idle", 8, 0.12f);
    if (_animAIdle) _animAIdle->retain();

    _animAChangeToB = loadAnimationFrames("Enemy/_BOSS_KuiLong/Boss_A_ChangeToB", "KL_A_ChangeToB", 12, 0.08f);
    if (_animAChangeToB) _animAChangeToB->retain();

    _animBMove = loadAnimationFrames("Enemy/_BOSS_KuiLong/Boss_B_Move", "KL_B_Move", 7, 0.10f);
    if (_animBMove) _animBMove->retain();

    _animBAttack = loadAnimationFrames("Enemy/_BOSS_KuiLong/Boss_B_Attack", "KL_B_Attack", 12, 0.10f);
    if (_animBAttack) _animBAttack->retain();

    _animBChangeToC = loadAnimationFrames("Enemy/_BOSS_KuiLong/Boss_B_ChangeToC", "KL_B_ChangeTo_C", 13, 0.10f);
    if (_animBChangeToC) _animBChangeToC->retain();

    _animBChengWuJie = loadAnimationFrames("Enemy/_BOSS_KuiLong/Boss_B_Skill2ChengWuJie", "KL_B_Skill2", 23, 0.10f);
    if (_animBChengWuJie) _animBChengWuJie->retain();

    // 加载承三身动画
    _animCSS_Start = loadAnimationFrames("Enemy/_BOSS_KuiLong/Boss_B_Skill1ChengSanShen_Start", "KL_B_Skill1_Start", 10, 0.1f); // 假设10帧
    if (_animCSS_Start) _animCSS_Start->retain();

    _animCSS_Idle = loadAnimationFrames("Enemy/_BOSS_KuiLong/Boss_B_Skill1ChengSanShen_Idle", "KL_B_Skill1_Idle", 8, 0.1f); // 假设8帧
    if (_animCSS_Idle) _animCSS_Idle->retain();

    _animCSS_End = loadAnimationFrames("Enemy/_BOSS_KuiLong/Boss_B_Skill1ChengSanShen_End", "KL_B_Skill1_End", 10, 0.1f); // 假设10帧
    if (_animCSS_End) _animCSS_End->retain();
}

bool KuiLongBoss::isPoisonable() const
{
    // 阶段 A、转场、承三身阶段免疫毒
    return !(_phase == PHASE_A || _phase == TRANSITION_A_TO_B || _phase == SKILL_CHENG_SAN_SHEN);
}

void KuiLongBoss::update(float dt)
{
    Enemy::update(dt);

    // 承三身阶段逻辑
    if (_phase == SKILL_CHENG_SAN_SHEN)
    {
        updateChengSanShen(dt);
        // 在此阶段不执行常规 update 逻辑（如生成 NiLuFire 等，如果需要暂停的话）
        // 但 NiLuFire 生成逻辑在下方，如果想暂停可以加判断
    }

    if (_phase == PHASE_A)
    {
        _phaseTimer += dt;
        if (_phaseTimer >= _phaseADuration)
        {
            _phase = TRANSITION_A_TO_B;
            _phaseTimer = 0.0f;
            this->removeStealthSource((void*)this);

            if (_animAChangeToB && _sprite)
            {
                _sprite->stopActionByTag(KUI_LONG_MOVE_TAG);
                _moveAnimPlaying = false;

                auto animate = Animate::create(_animAChangeToB);
                auto onFinish = CallFunc::create([this]() {
                    this->_phase = PHASE_B;
                    if (this->_animBMove && this->_sprite)
                    {
                        auto bmoveAnimate = Animate::create(this->_animBMove);
                        auto repeat = RepeatForever::create(bmoveAnimate);
                        repeat->setTag(KUI_LONG_MOVE_TAG);
                        this->_sprite->runAction(repeat);
                        this->_moveAnimPlaying = true;
                    }
                });
                auto seq = Sequence::create(animate, onFinish, nullptr);
                seq->setTag(KUI_LONG_CHANGE_TAG);
                _sprite->runAction(seq);
            }
            else
            {
                this->_phase = PHASE_B;
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

    // UI 更新
    if (_bossHPBar) {
        float percent = (getMaxHP() > 0) ? (static_cast<float>(getHP()) / static_cast<float>(getMaxHP()) * 100.0f) : 0.0f;
        _bossHPBar->setPercent(percent);
    }
    if (_bossHPLabel) {
        char buf[64];
        sprintf(buf, "%d/%d", getHP(), getMaxHP());
        _bossHPLabel->setString(buf);
    }

    // 技能冷却
    if (_skillCooldownTimer < _skillCooldown) {
        _skillCooldownTimer += dt;
        if (_skillCooldownTimer > _skillCooldown) _skillCooldownTimer = _skillCooldown;
    }

    // NiLuFire 生成 (承三身阶段是否生成？题目未明确，假设不生成或保持原样。这里保持原样，除非死亡)
    if (_currentState != EntityState::DIE && _phase != SKILL_CHENG_SAN_SHEN) // 假设承三身期间不生成小怪
    {
        _niluSpawnTimer += dt;
        if (_niluSpawnTimer >= _niluSpawnInterval) {
            _niluSpawnTimer = 0.0f;
            trySpawnNiLuFires();
        }
    }
}

void KuiLongBoss::executeAI(Player* player, float dt)
{
    if (_phase == PHASE_A || _phase == TRANSITION_A_TO_B || _phase == SKILL_CHENG_SAN_SHEN) return;

    if (_phase == PHASE_B)
    {
        if (player && !this->_skillPlaying && this->canUseChengWuJie())
        {
            float distSqr = (player->getPosition() - this->getPosition()).lengthSquared();
            if (distSqr <= (_skillRange * _skillRange))
            {
                this->startChengWuJie(player);
                return;
            }
        }
        Enemy::executeAI(player, dt);
    }
}

void KuiLongBoss::attack()
{
    if (_skillPlaying || _phase == SKILL_CHENG_SAN_SHEN) return;
    if (_phase == PHASE_A || _phase == TRANSITION_A_TO_B) return;
    if (!canAttack()) return;

    setState(EntityState::ATTACK);
    resetAttackCooldown();

    if (_sprite) {
        auto moveAct = _sprite->getActionByTag(KUI_LONG_MOVE_TAG);
        if (moveAct) _sprite->stopAction(moveAct);
        _moveAnimPlaying = false;
    }
    this->stopActionByTag(KUI_LONG_WINDUP_TAG);
    if (_sprite) {
        auto prevWind = _sprite->getActionByTag(KUI_LONG_WINDUP_TAG);
        if (prevWind) _sprite->stopAction(prevWind);
    }

    float windup = this->getAttackWindup();
    auto delay = DelayTime::create(windup);
    auto startAttackAnim = CallFunc::create([this]() {
        Scene* running = Director::getInstance()->getRunningScene();
        if (running) {
            GameScene* gs = dynamic_cast<GameScene*>(running);
            if (gs) {
                Player* player = gs->getPlayer();
                if (player && player->getParent() && !player->isDead()) {
                    float distSqr = (player->getPosition() - this->getPosition()).lengthSquared();
                    if (distSqr <= (this->getAttackRange() * this->getAttackRange())) {
                        int dmg = this->getAttack();
                        player->takeDamage(dmg);
                        Node* parentForText = player->getParent() ? player->getParent() : running;
                        if (parentForText) {
                            FloatingText::show(parentForText, player->getPosition(), std::to_string(dmg), Color3B(220,20,20));
                        }
                    }
                }
            }
            std::vector<NiLuFire*> found;
            collectNiLuFiresRecursive(running, found);
            for (auto fire : found) {
                if (!fire) continue;
                if (!_roomBounds.equals(Rect::ZERO)) {
                    if (!_roomBounds.containsPoint(fire->getPosition())) continue;
                }
                fire->performAttackImmediate(this->getAttack());
            }
        }
        this->playAttackAnimation();
    });
    auto seq = Sequence::create(delay, startAttackAnim, nullptr);
    seq->setTag(KUI_LONG_WINDUP_TAG);
    this->runAction(seq);
}

void KuiLongBoss::playAttackAnimation()
{
    if (_skillPlaying || _phase == SKILL_CHENG_SAN_SHEN) return;
    if (_phase == PHASE_A || _phase == TRANSITION_A_TO_B) return;
    if (!_sprite) return;
    if (_currentState == EntityState::DIE) return;

    if (_currentState != EntityState::ATTACK) setState(EntityState::ATTACK);

    auto moveAct = _sprite->getActionByTag(KUI_LONG_MOVE_TAG);
    if (moveAct) _sprite->stopAction(moveAct);
    _moveAnimPlaying = false;

    auto wind = _sprite->getActionByTag(KUI_LONG_WINDUP_TAG);
    if (wind) _sprite->stopAction(wind);

    if (_animBAttack) {
        auto prevHit = _sprite->getActionByTag(KUI_LONG_HIT_TAG);
        if (prevHit) _sprite->stopAction(prevHit);

        _animBAttack->setRestoreOriginalFrame(true);
        auto animate = Animate::create(_animBAttack);
        animate->setTag(KUI_LONG_HIT_TAG);

        auto callback = CallFunc::create([this]() {
            if (_animBMove && _sprite) {
                auto frames = _animBMove->getFrames();
                if (!frames.empty()) _sprite->setSpriteFrame(frames.front()->getSpriteFrame());
            }
            Scene* running = Director::getInstance()->getRunningScene();
            if (running) {
                std::vector<NiLuFire*> found;
                collectNiLuFiresRecursive(running, found);
                for (auto fire : found) {
                    if (!fire) continue;
                    if (!_roomBounds.equals(Rect::ZERO)) {
                        if (!_roomBounds.containsPoint(fire->getPosition())) continue;
                    }
                    fire->performAttackImmediate(this->getAttack());
                }
            }
            if (_currentState != EntityState::DIE) setState(EntityState::IDLE);
        });
        auto seq = Sequence::create(animate, callback, nullptr);
        seq->setTag(KUI_LONG_HIT_TAG);
        _sprite->runAction(seq);
    }
}

void KuiLongBoss::takeDamage(int damage)
{
    if (_phase == PHASE_A || _phase == TRANSITION_A_TO_B || _phase == SKILL_CHENG_SAN_SHEN) return;

    int count = countNiLuFiresInRoom();
    float factor = powf(0.85f, static_cast<float>(count));
    int reduced = static_cast<int>(std::floor(static_cast<float>(damage) * factor + 0.0001f));

    Enemy::takeDamage(reduced);

    // 检查承三身触发
    checkChengSanShenTrigger();
}

int KuiLongBoss::takeDamageReported(int damage)
{
    if (_phase == PHASE_A || _phase == TRANSITION_A_TO_B || _phase == SKILL_CHENG_SAN_SHEN) return 0;

    int count = countNiLuFiresInRoom();
    float factor = powf(0.85f, static_cast<float>(count));
    int reduced = static_cast<int>(std::floor(static_cast<float>(damage) * factor + 0.0001f));

    int actual = Enemy::takeDamageReported(reduced);
    
    // 检查承三身触发
    checkChengSanShenTrigger();

    return actual;
}

void KuiLongBoss::checkChengSanShenTrigger()
{
    if (_phase != PHASE_B) return; // 只有在阶段B才能触发

    float hpPercent = (float)getHP() / (float)getMaxHP();

    if (!_threshold75Triggered && hpPercent <= 0.75f) {
        _threshold75Triggered = true;
        startChengSanShen();
    }
    else if (!_threshold50Triggered && hpPercent <= 0.50f) {
        _threshold50Triggered = true;
        startChengSanShen();
    }
    else if (!_threshold25Triggered && hpPercent <= 0.25f) {
        _threshold25Triggered = true;
        startChengSanShen();
    }
}

void KuiLongBoss::startChengSanShen()
{
    if (_phase == SKILL_CHENG_SAN_SHEN) return;

    _phase = SKILL_CHENG_SAN_SHEN;
    _chengSanShenTimer = 0.0f;
    
    // 停止所有当前动作
    stopAllActions();
    if (_sprite) _sprite->stopAllActions();
    _moveAnimPlaying = false;
    _skillPlaying = false; // 强制结束 ChengWuJie
    _skillDamageScheduled = false;

    // 进入隐身/无敌
    this->addStealthSource((void*)this);
    // 状态设为 SKILL 或 IDLE，防止移动
    setState(EntityState::SKILL);

    // 播放 Start -> Idle
    if (_sprite && _animCSS_Start)
    {
        auto startAnim = Animate::create(_animCSS_Start);
        auto playIdle = CallFunc::create([this](){
            if (_sprite && _animCSS_Idle) {
                auto idleAnim = Animate::create(_animCSS_Idle);
                auto repeat = RepeatForever::create(idleAnim);
                repeat->setTag(KUI_LONG_CSS_TAG);
                _sprite->runAction(repeat);
            }
            // 召唤 Boat
            if (!_roomBounds.equals(Rect::ZERO)) {
                auto boat = Boat::create();
                if (boat) {
                    boat->setRoomBounds(_roomBounds);
                    // 随机边界位置
                    float x, y;
                    int edge = cocos2d::random(0, 3);
                    switch(edge) {
                        case 0: // Top
                            x = cocos2d::random(_roomBounds.getMinX(), _roomBounds.getMaxX());
                            y = _roomBounds.getMaxY();
                            break;
                        case 1: // Bottom
                            x = cocos2d::random(_roomBounds.getMinX(), _roomBounds.getMaxX());
                            y = _roomBounds.getMinY();
                            break;
                        case 2: // Left
                            x = _roomBounds.getMinX();
                            y = cocos2d::random(_roomBounds.getMinY(), _roomBounds.getMaxY());
                            break;
                        case 3: // Right
                            x = _roomBounds.getMaxX();
                            y = cocos2d::random(_roomBounds.getMinY(), _roomBounds.getMaxY());
                            break;
                    }
                    boat->setPosition(Vec2(x, y));
                    
                    auto scene = Director::getInstance()->getRunningScene();
                    auto gs = dynamic_cast<GameScene*>(scene);
                    if (gs) gs->addEnemy(boat);
                    
                    _summonedBoat = boat;
                }
            }
        });
        auto seq = Sequence::create(startAnim, playIdle, nullptr);
        seq->setTag(KUI_LONG_CSS_TAG);
        _sprite->runAction(seq);
    }
}

void KuiLongBoss::updateChengSanShen(float dt)
{
    _chengSanShenTimer += dt;
    if (_chengSanShenTimer >= _chengSanShenDuration)
    {
        endChengSanShen();
    }
}

void KuiLongBoss::endChengSanShen()
{
    // 移除 Boat
    if (_summonedBoat && !_summonedBoat->getReferenceCount()) {
        _summonedBoat = nullptr; // 已被销毁
    }
    if (_summonedBoat) {
        _summonedBoat->forceDissipate();
        _summonedBoat = nullptr;
    }

    // 播放 End 动画
    if (_sprite) {
        _sprite->stopActionByTag(KUI_LONG_CSS_TAG);
        
        auto finishCallback = CallFunc::create([this](){
            // 恢复状态
            _phase = PHASE_B;
            this->removeStealthSource((void*)this);
            setState(EntityState::IDLE);
            
            // 恢复移动动画
            if (_animBMove && _sprite) {
                auto bmoveAnimate = Animate::create(_animBMove);
                auto repeat = RepeatForever::create(bmoveAnimate);
                repeat->setTag(KUI_LONG_MOVE_TAG);
                _sprite->runAction(repeat);
                _moveAnimPlaying = true;
            }
        });

        if (_animCSS_End) {
            auto endAnim = Animate::create(_animCSS_End);
            auto seq = Sequence::create(endAnim, finishCallback, nullptr);
            _sprite->runAction(seq);
        } else {
            finishCallback->execute();
        }
    } else {
        // 无 Sprite 兜底
        _phase = PHASE_B;
        this->removeStealthSource((void*)this);
        setState(EntityState::IDLE);
    }
}

// ... (startChengWuJie, die, move, destructor, loadAnimationFrames, canUseChengWuJie, resetChengWuJieCooldown 保持不变) ...

void KuiLongBoss::startChengWuJie(Player* target)
{
    if (_phase != PHASE_B) return;
    if (!target) return;
    if (!_sprite) return;
    if (!canUseChengWuJie()) return;

    _skillPlaying = true;
    setState(EntityState::SKILL);
    resetChengWuJieCooldown();

    auto moveAct = _sprite->getActionByTag(KUI_LONG_MOVE_TAG);
    if (moveAct) _sprite->stopAction(moveAct);
    _moveAnimPlaying = false;

    auto wind = _sprite->getActionByTag(KUI_LONG_WINDUP_TAG);
    if (wind) _sprite->stopAction(wind);
    auto prevHit = _sprite->getActionByTag(KUI_LONG_HIT_TAG);
    if (prevHit) _sprite->stopAction(prevHit);
    this->stopActionByTag(KUI_LONG_WINDUP_TAG);
    _sprite->stopActionByTag(KUI_LONG_CHANGE_TAG);

    if (_animBChengWuJie) {
        _animBChengWuJie->setRestoreOriginalFrame(true);
        auto animate = Animate::create(_animBChengWuJie);
        animate->setTag(KUI_LONG_SKILL_TAG);
        _sprite->runAction(animate);
    }

    this->stopActionByTag(KUI_LONG_SKILL_DAMAGE_TAG);
    this->_skillDamageScheduled = false;

    int dmgPerHit = this->_skillDamagePerHit;
    Vector<FiniteTimeAction*> acts;
    auto createHitFunc = [this, target, dmgPerHit]() {
        return CallFunc::create([this, target, dmgPerHit]() {
            if (!target) return;
            if (this->_currentState == EntityState::DIE) return;
            float distSqr = (target->getPosition() - this->getPosition()).lengthSquared();
            if (distSqr <= (this->_skillRange * this->_skillRange)) {
                target->takeDamage(dmgPerHit);
            }
        });
    };

    acts.pushBack(createHitFunc());
    acts.pushBack(DelayTime::create(0.5f));
    acts.pushBack(createHitFunc());
    acts.pushBack(DelayTime::create(0.5f));
    acts.pushBack(createHitFunc());
    acts.pushBack(DelayTime::create(0.5f));
    acts.pushBack(createHitFunc());
    acts.pushBack(DelayTime::create(1.0f));
    acts.pushBack(createHitFunc());

    acts.pushBack(CallFunc::create([this]() {
        this->_skillDamageScheduled = false;
        this->_skillPlaying = false;
        if (this->_currentState != EntityState::DIE) {
            this->setState(EntityState::IDLE);
        }
    }));

    FiniteTimeAction* seqHead = nullptr;
    if (!acts.empty()) {
        seqHead = acts.at(0);
        for (ssize_t i = 1; i < static_cast<ssize_t>(acts.size()); ++i) {
            seqHead = Sequence::create(seqHead, acts.at(i), nullptr);
        }
    }

    if (seqHead) {
        seqHead->setTag(KUI_LONG_SKILL_DAMAGE_TAG);
        this->_skillDamageScheduled = true;
        this->runAction(dynamic_cast<Action*>(seqHead));
    }
}

void KuiLongBoss::die()
{
    if (_currentState == EntityState::DIE) return;
    setState(EntityState::DIE);
    this->removeStealthSource((void*)this);

    // 移除 NiLuFire
    Scene* running = Director::getInstance()->getRunningScene();
    if (running) {
        std::vector<NiLuFire*> found;
        collectNiLuFiresRecursive(running, found);
        for (auto fire : found) {
            if (!fire) continue;
            if (!_roomBounds.equals(Rect::ZERO)) {
                if (!_roomBounds.containsPoint(fire->getPosition())) continue;
            }
            fire->stopAllActions();
            fire->unscheduleAllCallbacks();
            fire->setState(EntityState::DIE);
            fire->removeFromParentAndCleanup(true);
        }
    }
    
    // 移除 Boat (如果存在)
    if (_summonedBoat) {
        _summonedBoat->forceDissipate();
        _summonedBoat = nullptr;
    }

    if (_sprite) {
        _sprite->stopActionByTag(KUI_LONG_MOVE_TAG);
        _sprite->stopActionByTag(KUI_LONG_WINDUP_TAG);
        _sprite->stopActionByTag(KUI_LONG_HIT_TAG);
        _sprite->stopActionByTag(KUI_LONG_CHANGE_TAG);
        _sprite->stopActionByTag(KUI_LONG_CSS_TAG); // 停止承三身
    }

    if (_phase == PHASE_B && _animBChangeToC && _sprite) {
        // (保持原有的死亡转场逻辑)
        auto origFrames = _animBChangeToC->getFrames();
        float delayPerUnit = 0.1f;
        const ssize_t total = static_cast<ssize_t>(origFrames.size());
        const ssize_t splitIndex = 5;
        if (total >= splitIndex + 1) {
            Vector<SpriteFrame*> firstFrames;
            for (ssize_t i = 0; i < splitIndex && i < total; ++i) firstFrames.pushBack(origFrames.at(i)->getSpriteFrame());
            Vector<SpriteFrame*> secondFrames;
            for (ssize_t i = splitIndex; i < total; ++i) secondFrames.pushBack(origFrames.at(i)->getSpriteFrame());

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
            if (!seqActs.empty()) {
                current = seqActs.at(0);
                for (ssize_t i = 1; i < static_cast<ssize_t>(seqActs.size()); ++i) {
                    current = Sequence::create(static_cast<ActionInterval*>(current), static_cast<FiniteTimeAction*>(seqActs.at(i)), nullptr);
                }
            }
            if (current) {
                auto seq = dynamic_cast<Action*>(current);
                if (seq) {
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
    } else {
        Enemy::die();
        this->removeFromParentAndCleanup(true);
    }
}

void KuiLongBoss::move(const cocos2d::Vec2& direction, float dt)
{
    if (_currentState == EntityState::DIE || _skillPlaying || _currentState == EntityState::ATTACK || _phase == SKILL_CHENG_SAN_SHEN) return;

    if (direction.equals(cocos2d::Vec2::ZERO)) {
        if (_moveAnimPlaying && _sprite) {
            auto act = _sprite->getActionByTag(KUI_LONG_MOVE_TAG);
            if (act) _sprite->stopAction(act);
            _moveAnimPlaying = false;
        }
        return;
    }

    if (_sprite && !_moveAnimPlaying) {
        cocos2d::Animation* chosen = nullptr;
        if ((_phase == PHASE_B || _phase == PHASE_C) && _animBMove) chosen = _animBMove;
        else if (_animAIdle) chosen = _animAIdle;

        if (chosen) {
            auto animate = cocos2d::Animate::create(chosen);
            auto repeat = cocos2d::RepeatForever::create(animate);
            repeat->setTag(KUI_LONG_MOVE_TAG);
            _sprite->runAction(repeat);
            _moveAnimPlaying = true;
        }
    }
    this->setPosition(this->getPosition() + direction * dt);
}

KuiLongBoss::~KuiLongBoss()
{
    this->stopAllActions();
    if (_sprite) _sprite->stopAllActions();
    this->removeStealthSource((void*)this);

    if (_animAIdle)        { _animAIdle->release();        _animAIdle = nullptr; }
    if (_animAChangeToB)   { _animAChangeToB->release();   _animAChangeToB = nullptr; }
    if (_animBMove)        { _animBMove->release();        _animBMove = nullptr; }
    if (_animBAttack)      { _animBAttack->release();      _animBAttack = nullptr; }
    if (_animBChangeToC)   { _animBChangeToC->release();   _animBChangeToC = nullptr; }
    if (_animBChengWuJie)  { _animBChengWuJie->release();  _animBChengWuJie = nullptr; }
    if (_animCSS_Start)    { _animCSS_Start->release();    _animCSS_Start = nullptr; }
    if (_animCSS_Idle)     { _animCSS_Idle->release();     _animCSS_Idle = nullptr; }
    if (_animCSS_End)      { _animCSS_End->release();      _animCSS_End = nullptr; }

    if (_bossHPBar)    { _bossHPBar->removeFromParentAndCleanup(true); _bossHPBar = nullptr; }
    if (_bossHPLabel)  { _bossHPLabel->removeFromParentAndCleanup(true); _bossHPLabel = nullptr; }
    if (_skillSprite)  { _skillSprite->removeFromParentAndCleanup(true); _skillSprite = nullptr; }
}

cocos2d::Animation* KuiLongBoss::loadAnimationFrames(const std::string& folder, const std::string& prefix, int maxFrames, float delayPerUnit)
{
    Vector<SpriteFrame*> frames;
    for (int i = 1; i <= maxFrames; ++i) {
        char filename[512];
        sprintf(filename, "%s/%s_%04d.png", folder.c_str(), prefix.c_str(), i);
        SpriteFrame* frame = nullptr;
        auto s = Sprite::create(filename);
        if (s) frame = s->getSpriteFrame();
        if (!frame) {
            char basename[256];
            sprintf(basename, "%s_%04d.png", prefix.c_str(), i);
            frame = SpriteFrameCache::getInstance()->getSpriteFrameByName(basename);
        }
        if (frame) frames.pushBack(frame);
        else break;
    }
    if (frames.empty()) return nullptr;
    auto anim = Animation::createWithSpriteFrames(frames, delayPerUnit);
    return anim;
}

bool KuiLongBoss::canUseChengWuJie() const
{
    if (_phase != PHASE_B) return false;
    if (_skillPlaying) return false;
    if (_currentState == EntityState::DIE) return false;
    return (_skillCooldownTimer >= _skillCooldown);
}

void KuiLongBoss::resetChengWuJieCooldown()
{
    _skillCooldownTimer = 0.0f;
}