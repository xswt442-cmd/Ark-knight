#include "NiLuFire.h"
#include "cocos2d.h"
#include "Entities/Player/Player.h"
#include "UI/FloatingText.h"
#include "Scenes/GameScene.h"

USING_NS_CC;

static const char* LOG_TAG_NILU = "NiLuFire";

NiLuFire::NiLuFire()
    : _animAttack(nullptr)
    , _animBurn(nullptr)
    , _hpBar(nullptr)
    , _hpLabel(nullptr)
    , _lifetimeTimer(0.0f)
    , _protectTimer(5.0f) // 出场保护期 5s（期间可被设置为不响应某些效果）
    , _isProtected(true)
    , _lifeLimit(60.0f)
    , _attackDamage(0)
{
}

NiLuFire::~NiLuFire()
{
    if (_animAttack) { _animAttack->release(); _animAttack = nullptr; }
    if (_animBurn) { _animBurn->release(); _animBurn = nullptr; }

    if (_hpBar) { _hpBar->removeFromParentAndCleanup(true); _hpBar = nullptr; }
    if (_hpLabel) { _hpLabel->removeFromParentAndCleanup(true); _hpLabel = nullptr; }
}

bool NiLuFire::init()
{
    if (!Enemy::init()) return false;

    setEnemyType(EnemyType::MELEE);
    setMaxHP(1000); // 可按需调整
    setHP(getMaxHP());
    setMoveSpeed(0.0f); // 通常不移动
    setAttack(0); // 实际伤害使用 _attackDamage（在 spawn 时设置为 KuiLong 的攻击）
    setSightRange(300.0f);
    setAttackRange(120.0f);

    // 加载动画资源（仅 Attack/Burn）
    loadAnimations();

    // 绑定第一帧 sprite (优先 Burn 第一帧)
    if (_animBurn)
    {
        auto frames = _animBurn->getFrames();
        if (!frames.empty())
        {
            auto sprite = Sprite::createWithSpriteFrame(frames.front()->getSpriteFrame());
            this->bindSprite(sprite);
        }
    }
    else
    {
        auto sprite = Sprite::create();
        sprite->setTextureRect(Rect(0,0,32,32));
        sprite->setColor(Color3B::ORANGE);
        this->bindSprite(sprite);
    }

    // 绑定后启动燃烧循环动画（确保 _sprite 已存在）
    if (_animBurn && _sprite)
    {
        auto anim = Animate::create(_animBurn);
        auto repeat = RepeatForever::create(anim);
        repeat->setTag(0x9101);
        _sprite->runAction(repeat);
    }

    // 标记为无法被玩家普通子弹命中（使用 stealth 源）
    this->addStealthSource((void*)this);

    // 创建绿色血条（深绿色）
    createHPBar();

    GAME_LOG("%s: NiLuFire initialized (HP=%d)", LOG_TAG_NILU, getHP());
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
            // 使用与资源一致的路径
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
            // 使用与资源一致的路径
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

            // 这里不要假定 _sprite 已存在；循环播放在 init 中绑定后启动
        }
    }
}

void NiLuFire::createHPBar()
{
    if (!this->getParent()) return;

    float barWidth = 48.0f;
    float barHeight = 6.0f;

    _hpBar = ui::LoadingBar::create("UIs/StatusBars/Bars/HealthFill.png");
    if (_hpBar)
    {
        _hpBar->setAnchorPoint(Vec2(0.5f, 0.5f));
        _hpBar->setScaleX(barWidth / _hpBar->getContentSize().width);
        _hpBar->setScaleY(barHeight / _hpBar->getContentSize().height);
        _hpBar->setPosition(Vec2(0.0f, -(_sprite ? _sprite->getBoundingBox().size.height * 0.5f : 12.0f) - 6.0f));
        _hpBar->setColor(Color3B(0,120,0)); // 深绿色
        this->addChild(_hpBar, 1);
    }

    char buf[64];
    sprintf(buf, "%d/%d", getHP(), getMaxHP());
    _hpLabel = Label::createWithSystemFont(buf, "Arial", 10);
    if (_hpLabel)
    {
        _hpLabel->setAnchorPoint(Vec2(0.5f, 0.5f));
        _hpLabel->setTextColor(Color4B::WHITE);
        _hpLabel->setPosition(Vec2(0.0f, -(_sprite ? _sprite->getBoundingBox().size.height * 0.5f : 12.0f) - 14.0f));
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
        if (_protectTimer <= 0.0f)
        {
            _isProtected = false;
        }
    }

    _lifetimeTimer += dt;
    if (_lifetimeTimer >= _lifeLimit)
    {
        // 自爆攻击并销毁
        performSelfDestruct();
        return;
    }

    updateHPBar();
}

void NiLuFire::onHealedByPlayer(int healAmount)
{
    // 玩家治疗时对 NiLu 扣血（绕过普通免疫）
    if (healAmount <= 0) return;

    int oldHP = getHP();
    int newHP = oldHP - healAmount;
    setHP(std::max(0, newHP));

    if (this->getParent())
    {
        FloatingText::show(this->getParent(), this->getPosition(), std::to_string(healAmount), Color3B(50,200,50));
    }

    if (getHP() <= 0)
    {
        // 直接让基类处理死亡（不触发特殊 NiLu 自爆）
        this->die();
        this->removeFromParentAndCleanup(true);
    }
}

void NiLuFire::performAttackImmediate(int damage)
{
    if (_currentState == EntityState::DIE) return;

    // 播放攻击动画（若有）
    if (_animAttack && _sprite)
    {
        auto animate = Animate::create(_animAttack);
        animate->setTag(0x9120);
        auto cb = CallFunc::create([this]() {
            // do nothing visual after
        });
        auto seq = Sequence::create(animate, cb, nullptr);
        _sprite->runAction(seq);
    }

    // 对玩家造成伤害（简单实现：如果玩家在十字范围内则造成伤害）
    Scene* running = Director::getInstance()->getRunningScene();
    if (!running) return;
    GameScene* gs = dynamic_cast<GameScene*>(running);
    if (!gs) return;

    Player* player = gs->getPlayer();
    if (!player) return;

    Vec2 pPos = player->getPosition();
    Vec2 cPos = this->getPosition();

    // 十字判定：允许在 x 轴 +/- 100 或 y 轴 +/- 100 并在对向轴宽度 40 内
    float arm = 100.0f;
    float halfWidth = 20.0f;

    bool hit = false;
    if (fabs(pPos.x - cPos.x) <= halfWidth && fabs(pPos.y - cPos.y) <= arm) hit = true;
    if (fabs(pPos.y - cPos.y) <= halfWidth && fabs(pPos.x - cPos.x) <= arm) hit = true;

    if (hit)
    {
        player->takeDamage(damage);
    }
}

void NiLuFire::performSelfDestruct()
{
    // 播放攻击动画一次并对十字范围造成巨大伤害（采用设计稿数值 3500）
    int bigDamage = 3500;

    // 同步播放攻击动画
    if (_animAttack && _sprite)
    {
        auto animate = Animate::create(_animAttack);
        animate->setTag(0x9130);
        auto cb = CallFunc::create([this, bigDamage]() {
            // 造成傷害（類似 performAttackImmediate，但對房間內所有玩家單位生效）
            Scene* running = Director::getInstance()->getRunningScene();
            if (!running) return;
            GameScene* gs = dynamic_cast<GameScene*>(running);
            if (!gs) return;
            Player* player = gs->getPlayer();
            if (!player) return;

            Vec2 pPos = player->getPosition();
            Vec2 cPos = this->getPosition();

            float arm = 100.0f;
            float halfWidth = 20.0f;

            bool hit = false;
            if (fabs(pPos.x - cPos.x) <= halfWidth && fabs(pPos.y - cPos.y) <= arm) hit = true;
            if (fabs(pPos.y - cPos.y) <= halfWidth && fabs(pPos.x - cPos.x) <= arm) hit = true;

            if (hit)
            {
                player->takeDamage(bigDamage);
            }

            // 释放完毕后自毁
            this->removeFromParentAndCleanup(true);
        });
        auto seq = Sequence::create(animate, cb, nullptr);
        _sprite->runAction(seq);
    }
    else
    {
        // 无动画情况下直接造成伤害并删除
        Scene* running = Director::getInstance()->getRunningScene();
        if (!running) return;
        GameScene* gs = dynamic_cast<GameScene*>(running);
        if (!gs) return;
        Player* player = gs->getPlayer();
        if (!player) return;

        Vec2 pPos = player->getPosition();
        Vec2 cPos = this->getPosition();

        float arm = 100.0f;
        float halfWidth = 20.0f;

        bool hit = false;
        if (fabs(pPos.x - cPos.x) <= halfWidth && fabs(pPos.y - cPos.y) <= arm) hit = true;
        if (fabs(pPos.y - cPos.y) <= halfWidth && fabs(pPos.x - cPos.x) <= arm) hit = true;

        if (hit)
        {
            player->takeDamage(bigDamage);
        }
        this->removeFromParentAndCleanup(true);
    }
}

void NiLuFire::takeDamage(int damage)
{
    // NiLuFire 对我方普通攻击/子弹免疫（治疗会调用 onHealedByPlayer 来扣血）
    GAME_LOG("%s: takeDamage ignored (incoming=%d)", LOG_TAG_NILU, damage);
    (void)damage;
    return;
}