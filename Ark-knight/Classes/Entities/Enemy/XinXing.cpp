#include "XinXing.h"
#include "Entities/Enemy/IronLance.h"
#include "Scenes/GameScene.h"
#include "Entities/Player/Player.h"
#include "cocos2d.h"
#include <cmath>
#include <vector>

USING_NS_CC;

static const int XINX_MOVE_ACTION_TAG    = 0xE101;
static const int XINX_HIT_ACTION_TAG     = 0xE102;
static const int XINX_WINDUP_ACTION_TAG  = 0xE103;
// 节点层面的风箱回退（与 sprite 上的 windup 区分）
static const int XINX_WINDUP_NODE_TAG   = 0xE104;

XinXing::XinXing()
    : _moveAnimation(nullptr)
    , _attackAnimation(nullptr)
    , _dieAnimation(nullptr)
    , _roomBounds(Rect::ZERO)
    , _hasRoomBounds(false)
{
}

XinXing::~XinXing()
{
    CC_SAFE_RELEASE(_moveAnimation);
    CC_SAFE_RELEASE(_attackAnimation);
    CC_SAFE_RELEASE(_dieAnimation);
}

bool XinXing::init()
{
    if (!Enemy::init())
        return false;

    setEnemyType(EnemyType::MELEE);

    setupAttributes();
    loadAnimations();

    // 使用移动动画第一帧作为初始精灵，并放大 1.5 倍
    if (_moveAnimation)
    {
        auto frames = _moveAnimation->getFrames();
        if (!frames.empty())
        {
            auto sprite = Sprite::createWithSpriteFrame(frames.front()->getSpriteFrame());
            sprite->setScale(1.5f);
            this->bindSprite(sprite);
        }
    }
    else
    {
        // 回退：确保一定有一个可见的占位 Sprite（避免看不到实体）
        // 直接创建纯色矩形 Sprite 作为占位（避免使用 RenderTexture::visit 导致的运行时崩溃）
        auto placeholder = Sprite::create();
        // 创建一个简单的纹理矩形表现（不依赖外部图片）
        placeholder->setTextureRect(Rect(0,0,32,32));
        placeholder->setColor(Color3B(200, 50, 50)); // 红色占位
        placeholder->setAnchorPoint(Vec2(0.5f, 0.5f));
        placeholder->setScale(1.5f);
        this->bindSprite(placeholder);
        GAME_LOG("XinXing::init - using simple placeholder sprite because no move animation loaded");
    }

    GAME_LOG("XinXing created");
    return true;
}

void XinXing::setupAttributes()
{
    // 基础属性（可按需微调）
    setMaxHP(6000);
    setHP(6000);
    setAttack(2000);            // 高伤害
    setMoveSpeed(120.0f);

    setSightRange(320.0f);
    setAttackRange(50.0f);

    setAttackCooldown(0.8f);  // 攻速快
    setAttackWindup(0.25f);   // 前摇短
}

static Vector<SpriteFrame*> tryLoadFramesFromPatterns(const std::vector<std::string>& patterns, int count)
{
    Vector<SpriteFrame*> frames;
    char filename[512];

    for (const auto& pat : patterns)
    {
        for (int i = 1; i <= count; ++i)
        {
            snprintf(filename, sizeof(filename), pat.c_str(), i);
            std::string fn(filename);
            auto s = Sprite::create(fn);
            if (s && s->getSpriteFrame())
            {
                frames.pushBack(s->getSpriteFrame());
            }
            else
            {
                GAME_LOG("XinXing::loadAnimations - failed to load frame: %s", fn.c_str());
            }
        }

        if (!frames.empty())
        {
            GAME_LOG("XinXing::loadAnimations - loaded %zu frames using pattern '%s'", frames.size(), pat.c_str());
            break; // 已找到一种可用的 pattern，停止尝试其他 pattern
        }
    }

    return frames;
}

void XinXing::loadAnimations()
{
    // 尝试若干可能的路径模式（兼容不同资源组织）
    std::vector<std::string> movePatterns = {
        "Enemy/XinXing&&Iron Lance/XinXing/XinXing_Move/XinXing_Move_%04d.png",
        "Enemy/XinXing&&IronXing_Move/XinXing_Move_%04d.png",
        "Enemy/XinXing/XinXing_Move/XinXing_Move_%04d.png",
        "Enemy/XinXing_Move/XinXing_Move_%04d.png"
    };

    Vector<SpriteFrame*> moveFrames = tryLoadFramesFromPatterns(movePatterns, 6);
    if (!moveFrames.empty())
    {
        _moveAnimation = Animation::createWithSpriteFrames(moveFrames, 0.10f);
        _moveAnimation->retain();
    }
    else
    {
        GAME_LOG("XinXing::loadAnimations - no move frames found for XinXing");
    }

    // 攻击动画（若无资源，不会阻塞）
    std::vector<std::string> atkPatterns = {
        "Enemy/XinXing&&Iron Lance/XinXing/XinXing_Attack/XinXing_Attack_%04d.png",
        "Enemy/XinXing&&IronXing_Attack/XinXing_Attack_%04d.png",
        "Enemy/XinXing/XinXing_Attack/XinXing_Attack_%04d.png",
        "Enemy/XinXing_Attack/XinXing_Attack_%04d.png"
    };
    Vector<SpriteFrame*> atkFrames = tryLoadFramesFromPatterns(atkPatterns, 9);
    if (!atkFrames.empty())
    {
        _attackAnimation = Animation::createWithSpriteFrames(atkFrames, 0.08f);
        _attackAnimation->retain();
    }
    else
    {
        GAME_LOG("XinXing::loadAnimations - no attack frames found for XinXing");
    }

    // 死亡动画（可选）
    std::vector<std::string> diePatterns = {
        "Enemy/XinXing&&Iron Lance/XinXing/XinXing_Die/XinXing_Die_%04d.png",
        "Enemy/XinXing&&IronXing_Die/XinXing_Die_%04d.png",
        "Enemy/XinXing/XinXing_Die/XinXing_Die_%04d.png",
        "Enemy/XinXing_Die/XinXing_Die_%04d.png"
    };
    Vector<SpriteFrame*> dieFrames = tryLoadFramesFromPatterns(diePatterns, 6);
    if (!dieFrames.empty())
    {
        _dieAnimation = Animation::createWithSpriteFrames(dieFrames, 0.12f);
        _dieAnimation->retain();
    }
    else
    {
        GAME_LOG("XinXing::loadAnimations - no die frames found for XinXing");
    }
}

void XinXing::update(float dt)
{
    if (_currentState == EntityState::DIE) return;

    Enemy::update(dt);

    // 房间边界限制
    if (_hasRoomBounds)
    {
        Vec2 pos = this->getPosition();
        pos.x = std::max(_roomBounds.getMinX(), std::min(pos.x, _roomBounds.getMaxX()));
        pos.y = std::max(_roomBounds.getMinY(), std::min(pos.y, _roomBounds.getMaxY()));
        this->setPosition(pos);
    }
}

void XinXing::executeAI(Player* player, float dt)
{
    // 使用基类的 AI/攻击风箱逻辑以保证攻击前摇与命中动画正确同步
    if (_currentState == EntityState::DIE || !_isAlive) return;
    Enemy::executeAI(player, dt);
}

void XinXing::attack()
{
    if (!canAttack()) return;

    setState(EntityState::ATTACK);
    resetAttackCooldown();

    // 先在节点层面注册一个风箱延迟回退（确保即便 playAttackAnimation 未被调用也不会一直卡在 ATTACK）
    float windup = getAttackWindup();
    // 先移除已有同 tag 的（避免重复）
    this->stopActionByTag(XINX_WINDUP_NODE_TAG);
    auto nodeDelay = Sequence::create(DelayTime::create(windup),
                                      CallFunc::create([this]() {
                                          if (_currentState == EntityState::ATTACK) setState(EntityState::IDLE);
                                      }), nullptr);
    nodeDelay->setTag(XINX_WINDUP_NODE_TAG);
    this->runAction(nodeDelay);

    if (_attackAnimation && _sprite)
    {
        // 停止移动循环
        auto moveAct = _sprite->getActionByTag(XINX_MOVE_ACTION_TAG);
        if (moveAct) _sprite->stopAction(moveAct);

        // 停止之前的前摇动作（如果有）
        auto prevWind = _sprite->getActionByTag(XINX_WINDUP_ACTION_TAG);
        if (prevWind) _sprite->stopAction(prevWind);

        // 播放前摇动画（使用攻击动画作为前摇视觉）
        _attackAnimation->setRestoreOriginalFrame(true);
        auto animate = Animate::create(_attackAnimation);
        animate->setTag(XINX_WINDUP_ACTION_TAG);
        _sprite->runAction(animate);

        // 注意：不要在这里再强制把状态置回 IDLE（节点层面的延时回退会处理逃跑场景；当 playAttackAnimation 被调用时会停止该回退）
    }
    else
    {
        // 没有动画时按风箱时长回退到 IDLE（兼容无视觉资源）
        this->stopActionByTag(XINX_WINDUP_NODE_TAG);
        auto fallback = Sequence::create(DelayTime::create(windup),
                                         CallFunc::create([this]() {
                                             if (_currentState == EntityState::ATTACK) setState(EntityState::IDLE);
                                         }), nullptr);
        fallback->setTag(XINX_WINDUP_NODE_TAG);
        this->runAction(fallback);
    }
}

void XinXing::playAttackAnimation()
{
    if (!_sprite) return;
    if (_currentState == EntityState::DIE) return;

    // 如果我们即将播放命中动画，先取消节点层面的风箱回退，避免在命中动画进行中被置回 IDLE
    this->stopActionByTag(XINX_WINDUP_NODE_TAG);

    if (_currentState != EntityState::ATTACK) setState(EntityState::ATTACK);

    auto moveAct = _sprite->getActionByTag(XINX_MOVE_ACTION_TAG);
    if (moveAct) _sprite->stopAction(moveAct);

    auto wind = _sprite->getActionByTag(XINX_WINDUP_ACTION_TAG);
    if (wind) _sprite->stopAction(wind);

    if (_attackAnimation)
    {
        auto prev = _sprite->getActionByTag(XINX_HIT_ACTION_TAG);
        if (prev) _sprite->stopAction(prev);

        _attackAnimation->setRestoreOriginalFrame(true);
        auto animate = Animate::create(_attackAnimation);
        // 将 tag 应用到外层 sequence，便于后续查询/停止
        auto seq = Sequence::create(animate, CallFunc::create([this]() {
                if (_moveAnimation && _sprite) {
                    auto frames = _moveAnimation->getFrames();
                    if (!frames.empty()) _sprite->setSpriteFrame(frames.front()->getSpriteFrame());
                }
                if (_currentState != EntityState::DIE) setState(EntityState::IDLE);
            }), nullptr);
        seq->setTag(XINX_HIT_ACTION_TAG);
        _sprite->runAction(seq);
    }
}

void XinXing::move(const Vec2& direction, float dt)
{
    if (_currentState == EntityState::ATTACK || _currentState == EntityState::DIE)
    {
        Character::move(Vec2::ZERO, dt);
        return;
    }

    const float STOP_THRESHOLD_SQ = 1.0f;
    if (direction.lengthSquared() <= STOP_THRESHOLD_SQ)
    {
        if (_sprite)
        {
            auto act = _sprite->getActionByTag(XINX_MOVE_ACTION_TAG);
            if (act) _sprite->stopAction(act);

            if (_moveAnimation)
            {
                auto frames = _moveAnimation->getFrames();
                if (!frames.empty()) _sprite->setSpriteFrame(frames.front()->getSpriteFrame());
            }
        }
        Character::move(Vec2::ZERO, dt);
        return;
    }

    Vec2 dirNorm = direction.getNormalized();
    Character::move(dirNorm, dt);

    if (_sprite && _moveAnimation)
    {
        _sprite->setFlippedX(dirNorm.x < 0.0f);

        if (!_sprite->getActionByTag(XINX_MOVE_ACTION_TAG))
        {
            auto animate = Animate::create(_moveAnimation);
            auto repeat = RepeatForever::create(animate);
            repeat->setTag(XINX_MOVE_ACTION_TAG);
            _sprite->runAction(repeat);
        }
    }
}

void XinXing::die()
{
    // 调用基类以处理红色标记（KongKaZi）等通用视觉与生成功能
    Enemy::die();

    // 防止重复处理
    if (_currentState == EntityState::DIE && !_isAlive) return;

    setState(EntityState::DIE);
    _isAlive = false;

    this->stopAllActions();
    if (_sprite) { _sprite->stopAllActions(); _sprite->setVisible(true); _sprite->setOpacity(255); }

    // 播放死亡动画并在结束后生成 IronLance（若资源存在则用动画）
    auto finalizeSpawn = [this]() {
        Vec2 basePos = this->getPosition();
        const float PI_F = 3.14159265358979323846f;
        for (int i = 0; i < 3; ++i)
        {
            auto il = IronLance::create();
            if (!il) continue;

            float angle = (static_cast<float>(i) / 3.0f) * (2.0f * PI_F);
            float r = 24.0f + 8.0f * i;
            Vec2 spawnPos = basePos + Vec2(std::cos(angle) * r, std::sin(angle) * r);
            il->setPosition(spawnPos);

            if (_hasRoomBounds) il->setRoomBounds(_roomBounds);

            // 注册到 GameScene，保证房间管理与边界设置一致
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
        }

        this->removeFromParent();
    };

    if (_dieAnimation && _sprite)
    {
        auto animate = Animate::create(_dieAnimation);
        auto fade = FadeOut::create(0.4f);
        auto seq = Sequence::create(animate, fade, CallFunc::create(finalizeSpawn), nullptr);
        _sprite->runAction(seq);
    }
    else
    {
        finalizeSpawn();
    }
}