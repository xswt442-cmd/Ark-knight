// 允许奎隆触发并修复自爆回调崩溃
// 同时在造成伤害时显示浅蓝绿色浮动数字
#include "NiLuFire.h"
#include "cocos2d.h"
#include "Entities/Player/Player.h"
#include "UI/FloatingText.h"
#include "Scenes/GameScene.h"
#include "Core/Constants.h" // 用于 ZOrder

USING_NS_CC;

static const char* LOG_TAG_NILU = "NiLuFire";

#define NILU_DEBUG_DRAW 1

static void showDebugCross(cocos2d::Node* attachTarget, const cocos2d::Vec2& worldPos, float arm, float halfWidth, float duration)
{
#if NILU_DEBUG_DRAW
    if (!attachTarget) return;
    Vec2 center = attachTarget->convertToNodeSpace(worldPos);

    DrawNode* dn = DrawNode::create();
    Vec2 v1 = Vec2(center.x - halfWidth, center.y - arm);
    Vec2 v2 = Vec2(center.x + halfWidth, center.y + arm);
    Vec2 h1 = Vec2(center.x - arm, center.y - halfWidth);
    Vec2 h2 = Vec2(center.x + arm, center.y + halfWidth);

    Color4F color(1.0f, 0.2f, 0.2f, 0.6f);
    dn->drawSolidRect(v1, v2, color);
    dn->drawSolidRect(h1, h2, color);

    float z = static_cast<float>(Constants::ZOrder::FLOOR + 6);
    dn->setGlobalZOrder(z);
    attachTarget->addChild(dn, 1000);

    auto seq = Sequence::create(DelayTime::create(duration), RemoveSelf::create(), nullptr);
    dn->runAction(seq);
#endif
}

NiLuFire::NiLuFire()
    : _animAttack(nullptr)
    , _animBurn(nullptr)
    , _hpBar(nullptr)
    , _hpLabel(nullptr)
    , _lifetimeTimer(0.0f)
    , _protectTimer(5.0f) // 出场保护期 5s
    , _isProtected(true)
    , _lifeLimit(60.0f)
    , _attackDamage(0)
    , _isPerformingAttack(false)
{
}

NiLuFire::~NiLuFire()
{
    this->stopAllActions();
    if (_sprite) _sprite->stopAllActions();

    if (_animAttack) { _animAttack->release(); _animAttack = nullptr; }
    if (_animBurn) { _animBurn->release(); _animBurn = nullptr; }

    if (_hpBar) { _hpBar->removeFromParentAndCleanup(true); _hpBar = nullptr; }
    if (_hpLabel) { _hpLabel->removeFromParentAndCleanup(true); _hpLabel = nullptr; }
}

bool NiLuFire::init()
{
    if (!Enemy::init()) return false;

    setEnemyType(EnemyType::MELEE);
    setMaxHP(2000);    // 修改：血量上限设为 2000
    setHP(0);          // 初始血量为 0（如设计要求）
    setMoveSpeed(0.0f);
    setAttack(0);      // 物理攻击值不用于 NiLu，使用 performAttackImmediate 的参数
    setSightRange(300.0f);
    setAttackRange(120.0f);

    // 载入动画资源（Attack / Burning）
    loadAnimations();

    // 绑定 Burning 的第一帧为默认精灵
    if (_animBurn)
    {
        auto frames = _animBurn->getFrames();
        if (!frames.empty())
        {
            auto sprite = Sprite::createWithSpriteFrame(frames.front()->getSpriteFrame());
            this->bindSprite(sprite);
        }
    }
    if (!_sprite)
    {
        auto sprite = Sprite::create();
        sprite->setTextureRect(Rect(0, 0, 32, 32));
        sprite->setColor(Color3B::ORANGE);
        this->bindSprite(sprite);
    }

    // 启动燃烧循环（Burning 状态相当于 Idle，不会做伤害判定）
    if (_animBurn && _sprite)
    {
        auto anim = Animate::create(_animBurn);
        anim->setTag(0x9201);
        auto repeat = RepeatForever::create(anim);
        repeat->setTag(0x9101);
        _sprite->runAction(repeat);
    }

    // 添加 stealth source，防止我方普通子弹/攻击命中（与原逻辑一致）
    this->addStealthSource((void*)this);

    // 创建蓝色血条并置于地板层之上（world-space）
    createHPBar();

    GAME_LOG("%s: NiLuFire initialized (HP=%d/%d)", LOG_TAG_NILU, getHP(), getMaxHP());
    return true;
}

void NiLuFire::loadAnimations()
{
    // Attack
    {
        Vector<SpriteFrame*> frames;
        for (int i = 1; i <= 6; ++i)
        {
            char filename[512];
            sprintf(filename, "Enemy/NiLu Fire/NiLu Fire_Attack/NiLuFire_Attack_%04d.png", i);
            SpriteFrame* frame = nullptr;
            auto s = Sprite::create(filename);
            if (s) frame = s->getSpriteFrame();
            if (!frame)
            {
                char basename[256];
                sprintf(basename, "NiLuFire_Attack_%04d.png", i);
                frame = SpriteFrameCache::getInstance()->getSpriteFrameByName(basename);
            }
            if (frame) frames.pushBack(frame);
            else break;
        }
        if (!frames.empty())
        {
            _animAttack = Animation::createWithSpriteFrames(frames, 0.08f);
            _animAttack->retain();
        }
    }

    // Burning / Idle
    {
        Vector<SpriteFrame*> frames;
        for (int i = 1; i <= 4; ++i)
        {
            char filename[512];
            sprintf(filename, "Enemy/NiLu Fire/NiLu Fire_Burning/NiLuFire_Burning_%04d.png", i);
            SpriteFrame* frame = nullptr;
            auto s = Sprite::create(filename);
            if (s) frame = s->getSpriteFrame();
            if (!frame)
            {
                char basename[256];
                sprintf(basename, "NiLuFire_Burn_%04d.png", i);
                frame = SpriteFrameCache::getInstance()->getSpriteFrameByName(basename);
            }
            if (frame) frames.pushBack(frame);
            else break;
        }
        if (!frames.empty())
        {
            _animBurn = Animation::createWithSpriteFrames(frames, 0.12f);
            _animBurn->retain();
        }
    }
}

void NiLuFire::createHPBar()
{
    // 为了确保 world-space 的可见性，直接作为 NiLuFire 的子节点，但设置 global z-order 到地板之上
    float barWidth = 48.0f;
    float barHeight = 6.0f;
    float bgGlobalZ = static_cast<float>(Constants::ZOrder::FLOOR + 1);
    float fillGlobalZ = static_cast<float>(Constants::ZOrder::FLOOR + 2);
    float labelGlobalZ = static_cast<float>(Constants::ZOrder::FLOOR + 3);

    // 使用 LoadingBar 做填充
    _hpBar = ui::LoadingBar::create("UIs/StatusBars/Bars/HealthFill.png");
    if (_hpBar)
    {
        _hpBar->setAnchorPoint(Vec2(0.5f, 0.5f));
        _hpBar->setScaleX(barWidth / _hpBar->getContentSize().width);
        _hpBar->setScaleY(barHeight / _hpBar->getContentSize().height);
        // 向上移动一些（靠近精灵）: 原先 -6，现在改为 -2，让血条更靠近精灵
        _hpBar->setPosition(Vec2(0.0f, -(_sprite ? _sprite->getBoundingBox().size.height * 0.5f : 12.0f) - 2.0f));
        _hpBar->setColor(Color3B(64, 128, 255)); // 蓝色血条
        _hpBar->setGlobalZOrder(fillGlobalZ);
        this->addChild(_hpBar, 1);
    }

    char buf[64];
    sprintf(buf, "%d/%d", getHP(), getMaxHP());
    _hpLabel = Label::createWithSystemFont(buf, "Arial", 10);
    if (_hpLabel)
    {
        _hpLabel->setAnchorPoint(Vec2(0.5f, 0.5f));
        _hpLabel->setTextColor(Color4B::WHITE);
        // 标签也上移一点（原 -14 -> -10）
        _hpLabel->setPosition(Vec2(0.0f, -(_sprite ? _sprite->getBoundingBox().size.height * 0.5f : 12.0f) - 10.0f));
        _hpLabel->setGlobalZOrder(labelGlobalZ);
        this->addChild(_hpLabel, 2);
    }
}

void NiLuFire::updateHPBar()
{
    if (_hpBar)
    {
        float percent = (getMaxHP() > 0) ? (static_cast<float>(getHP()) / static_cast<float>(getMaxHP()) * 100.0f) : 0.0f;
        _hpBar->setPercent(percent);
    }
    if (_hpLabel)
    {
        char buf[64];
        sprintf(buf, "%d/%d", getHP(), getMaxHP());
        _hpLabel->setString(buf);
    }
}

void NiLuFire::setRoomBounds(const Rect& bounds)
{
    _roomBounds = bounds;
}

void NiLuFire::update(float dt)
{
    Enemy::update(dt);

    if (_isProtected)
    {
        _protectTimer -= dt;
        if (_protectTimer <= 0.0f) _isProtected = false;
    }

    _lifetimeTimer += dt;
    if (_lifetimeTimer >= _lifeLimit)
    {
        // 到时间自爆（播放一次攻击动画并造成十字伤害后消失）
        performSelfDestruct();
        return;
    }

    // Burning 状态：不主动攻击 / 不造成伤害（即使玩家在范围内也不造成）
    // 血条更新
    updateHPBar();
}

void NiLuFire::onHealedByPlayer(int healAmount)
{
    if (healAmount <= 0) return;

    int oldHP = getHP();
    int newHP = oldHP + healAmount;
    if (newHP > getMaxHP()) newHP = getMaxHP();
    setHP(newHP);

    if (this->getParent())
    {
        // 显示蓝色浮动数值
        FloatingText::show(this->getParent(), this->getPosition(), std::to_string(healAmount), Color3B(100,180,255));
    }

    // 立即刷新血条显示（修复“回血未能即时看到”的问题）
    updateHPBar();

    // 小的视觉提示（可选）：短暂放大血条以示变化
    if (_hpBar)
    {
        _hpBar->stopAllActions();
        auto sx = _hpBar->getScaleX();
        auto sy = _hpBar->getScaleY();
        auto s1 = ScaleTo::create(0.06f, sx * 1.15f, sy);
        auto s2 = ScaleTo::create(0.12f, sx, sy);
        _hpBar->runAction(Sequence::create(s1, s2, nullptr));
    }

    // 如果血量回满，则立即移除（不触发自爆伤害）
    if (getHP() >= getMaxHP())
    {
        // 清理并移除
        this->stopAllActions();
        this->unscheduleAllCallbacks();
        this->removeFromParentAndCleanup(true);
    }
}

void NiLuFire::performAttackImmediate(int damage)
{
    if (_currentState == EntityState::DIE) return;

    // 仅当没有显式传入 damage（damage <= 0）时，保护期生效。
    // 这样 KuiLong 在调用时传入非0 damage 可强制触发尼卢火攻击（符合需求）。
    if (_isProtected && damage <= 0)
    {
        GAME_LOG("%s: performAttackImmediate ignored due to protect (incoming=%d)", LOG_TAG_NILU, damage);
        return;
    }

    if (damage > 0) _attackDamage = damage;

    if (!_animAttack)
    {
        GAME_LOG("%s: performAttackImmediate called but _animAttack missing - no damage applied", LOG_TAG_NILU);
        return;
    }

    auto frames = _animAttack->getFrames();
    if (frames.empty())
    {
        GAME_LOG("%s: performAttackImmediate called but _animAttack has 0 frames", LOG_TAG_NILU);
        return;
    }

    // 标记正在攻击（仅作为状态标记）
    _isPerformingAttack = true;

    // 为了在回调中安全访问 this，保留自身，回调结束时 release
    this->retain();

    // 捕获当前必要数据
    int usedDamage = (damage > 0) ? damage : (_attackDamage > 0 ? _attackDamage : this->getAttack());
    Node* attachTarget = this->getParent();
    if (!attachTarget) attachTarget = this;

    Vec2 worldPos;
    if (this->getParent())
        worldPos = this->getParent()->convertToWorldSpace(this->getPosition());
    else
        worldPos = this->getPosition();

    // 创建临时攻击精灵并挂到父节点/场景层以保证可见
    Sprite* atkSprite = Sprite::createWithSpriteFrame(frames.front()->getSpriteFrame());
    if (!atkSprite)
    {
        _isPerformingAttack = false;
        this->release();
        return;
    }
    atkSprite->setAnchorPoint(Vec2(0.5f, 0.5f));

    Vec2 targetPosInAttach = attachTarget->convertToNodeSpace(worldPos);
    atkSprite->setPosition(targetPosInAttach);

    if (_sprite) atkSprite->setScale(_sprite->getScale());

    float atkGlobalZ = static_cast<float>(Constants::ZOrder::FLOOR + 4);
    atkSprite->setGlobalZOrder(atkGlobalZ);
    attachTarget->addChild(atkSprite, 10);

    // debug 可视化与日志
    float animDuration = _animAttack->getDelayPerUnit() * static_cast<float>(_animAttack->getFrames().size());
    showDebugCross(attachTarget, worldPos, 100.0f, 20.0f, std::max(0.2f, animDuration));
    GAME_LOG("%s: performAttackImmediate spawn atkSprite at world(%.1f,%.1f) attach=%p frames=%zd duration=%.2f",
        LOG_TAG_NILU, worldPos.x, worldPos.y, (void*)attachTarget, frames.size(), animDuration);

    // 安全捕获：用 self 指针（已 retain）
    NiLuFire* self = this;
    auto animate = Animate::create(_animAttack);
    auto cb = CallFunc::create([self, atkSprite, usedDamage]() {
        // 在回调中不要假定 self 未被删除（retain 保证其期间有效）
        Scene* running = Director::getInstance()->getRunningScene();
        if (running)
        {
            GameScene* gs = dynamic_cast<GameScene*>(running);
            if (gs)
            {
                Player* player = gs->getPlayer();
                if (player)
                {
                    // 判定使用世界坐标比较（player 位置用 world）
                    Vec2 pWorld = (player->getParent()) ? player->getParent()->convertToWorldSpace(player->getPosition()) : player->getPosition();
                    Vec2 cWorld = (self->getParent()) ? self->getParent()->convertToWorldSpace(self->getPosition()) : self->getPosition();

                    bool hit = false;
                    if (fabs(pWorld.x - cWorld.x) <= 20.0f && fabs(pWorld.y - cWorld.y) <= 100.0f) hit = true;
                    if (fabs(pWorld.y - cWorld.y) <= 20.0f && fabs(pWorld.x - cWorld.x) <= 100.0f) hit = true;

                    if (hit && usedDamage > 0)
                    {
                        player->takeDamage(usedDamage);

                        // 显示浅蓝绿色浮动数字以便区分尼卢火伤害
                        Node* parentForText = player->getParent() ? player->getParent() : running;
                        if (parentForText)
                        {
                            FloatingText::show(parentForText, player->getPosition(), std::to_string(usedDamage), Color3B(100,220,180));
                        }
                    }
                }
            }
        }

        if (atkSprite) atkSprite->removeFromParentAndCleanup(true);

        // 清理标记并 release self
        if (self) self->_isPerformingAttack = false;
        if (self) self->release();
    });

    auto seq = Sequence::create(animate, cb, nullptr);
    atkSprite->runAction(seq);
}

void NiLuFire::performSelfDestruct()
{
    if (_currentState == EntityState::DIE) return;

    // 自爆使用记录的攻击力或 fallback
    int dmg = (_attackDamage > 0) ? _attackDamage : this->getAttack();
    if (dmg <= 0) dmg = 3500; // 兜底值

    if (_animAttack)
    {
        auto frames = _animAttack->getFrames();
        if (!frames.empty())
        {
            Sprite* atkSprite = Sprite::createWithSpriteFrame(frames.front()->getSpriteFrame());
            if (!atkSprite)
            {
                // 无法创建精灵，直接判伤并立即移除
                Scene* running = Director::getInstance()->getRunningScene();
                if (running)
                {
                    GameScene* gs = dynamic_cast<GameScene*>(running);
                    if (gs)
                    {
                        Player* player = gs->getPlayer();
                        if (player)
                        {
                            Vec2 pWorld = (player->getParent()) ? player->getParent()->convertToWorldSpace(player->getPosition()) : player->getPosition();
                            Vec2 cWorld = (this->getParent()) ? this->getParent()->convertToWorldSpace(this->getPosition()) : this->getPosition();

                            bool hit = false;
                            if (fabs(pWorld.x - cWorld.x) <= 20.0f && fabs(pWorld.y - cWorld.y) <= 100.0f) hit = true;
                            if (fabs(pWorld.y - cWorld.y) <= 20.0f && fabs(pWorld.x - cWorld.x) <= 100.0f) hit = true;

                            if (hit && dmg > 0)
                            {
                                player->takeDamage(dmg);
                                Node* parentForText = player->getParent() ? player->getParent() : running;
                                if (parentForText)
                                {
                                    FloatingText::show(parentForText, player->getPosition(), std::to_string(dmg), Color3B(100,220,180));
                                }
                            }
                        }
                    }
                }
                this->removeFromParentAndCleanup(true);
                return;
            }

            // retain self 保证回调期间有效
            this->retain();
            NiLuFire* self = this;

            Node* attachTarget = this->getParent();
            if (!attachTarget) attachTarget = this;

            Vec2 worldPos;
            if (this->getParent())
                worldPos = this->getParent()->convertToWorldSpace(this->getPosition());
            else
                worldPos = this->getPosition();

            Vec2 targetPosInAttach = attachTarget->convertToNodeSpace(worldPos);
            atkSprite->setPosition(targetPosInAttach);
            if (_sprite) atkSprite->setScale(_sprite->getScale());
            float atkGlobalZ = static_cast<float>(Constants::ZOrder::FLOOR + 4);
            atkSprite->setGlobalZOrder(atkGlobalZ);
            attachTarget->addChild(atkSprite, 10);

            float animDuration = _animAttack->getDelayPerUnit() * static_cast<float>(_animAttack->getFrames().size());
            showDebugCross(attachTarget, worldPos, 100.0f, 20.0f, std::max(0.2f, animDuration));

            auto animate = Animate::create(_animAttack);
            auto cb = CallFunc::create([self, atkSprite, dmg]() {
                Scene* running = Director::getInstance()->getRunningScene();
                if (running)
                {
                    GameScene* gs = dynamic_cast<GameScene*>(running);
                    if (gs)
                    {
                        Player* player = gs->getPlayer();
                        if (player)
                        {
                            Vec2 pWorld = (player->getParent()) ? player->getParent()->convertToWorldSpace(player->getPosition()) : player->getPosition();
                            Vec2 cWorld = (self->getParent()) ? self->getParent()->convertToWorldSpace(self->getPosition()) : self->getPosition();

                            bool hit = false;
                            if (fabs(pWorld.x - cWorld.x) <= 20.0f && fabs(pWorld.y - cWorld.y) <= 100.0f) hit = true;
                            if (fabs(pWorld.y - cWorld.y) <= 20.0f && fabs(pWorld.x - cWorld.x) <= 100.0f) hit = true;

                            if (hit && dmg > 0)
                            {
                                player->takeDamage(dmg);
                                Node* parentForText = player->getParent() ? player->getParent() : running;
                                if (parentForText)
                                {
                                    FloatingText::show(parentForText, player->getPosition(), std::to_string(dmg), Color3B(100,220,180));
                                }
                            }
                        }
                    }
                }
                if (atkSprite) atkSprite->removeFromParentAndCleanup(true);

                // 移除自身并 release（release 在 remove 之后依然安全）
                if (self)
                {
                    self->removeFromParentAndCleanup(true);
                    self->release();
                }
            });
            auto seq = Sequence::create(animate, cb, nullptr);
            atkSprite->runAction(seq);
            return;
        }
    }

    // 无动画回退：直接判定伤害并删除
    Scene* running = Director::getInstance()->getRunningScene();
    if (running)
    {
        GameScene* gs = dynamic_cast<GameScene*>(running);
        if (gs)
        {
            Player* player = gs->getPlayer();
            if (player)
            {
                Vec2 pWorld = (player->getParent()) ? player->getParent()->convertToWorldSpace(player->getPosition()) : player->getPosition();
                Vec2 cWorld = (this->getParent()) ? this->getParent()->convertToWorldSpace(this->getPosition()) : this->getPosition();

                bool hit = false;
                if (fabs(pWorld.x - cWorld.x) <= 20.0f && fabs(pWorld.y - cWorld.y) <= 100.0f) hit = true;
                if (fabs(pWorld.y - cWorld.y) <= 20.0f && fabs(pWorld.x - cWorld.x) <= 100.0f) hit = true;

                if (hit && dmg > 0)
                {
                    player->takeDamage(dmg);
                    Node* parentForText = player->getParent() ? player->getParent() : running;
                    if (parentForText)
                    {
                        FloatingText::show(parentForText, player->getPosition(), std::to_string(dmg), Color3B(100,220,180));
                    }
                }
            }
        }
    }
    this->removeFromParentAndCleanup(true);
}

void NiLuFire::takeDamage(int damage)
{
    // NiLuFire 对我方普通攻击/子弹免疫（治疗回满由 onHealedByPlayer 处理）
    GAME_LOG("%s: takeDamage ignored (incoming=%d)", LOG_TAG_NILU, damage);
    (void)damage;
    return;
}