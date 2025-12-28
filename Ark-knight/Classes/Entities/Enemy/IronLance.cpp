#include "IronLance.h"
#include "cocos2d.h"
#include "Entities/Player/Player.h"
#include <vector>
#include <string>

USING_NS_CC;

static const int IRONL_MOVE_ACTION_TAG = 0xD201;

// 辅助：尝试多个路径 pattern 加载帧（与 XinXing 的实现风格保持一致）
static Vector<SpriteFrame*> tryLoadFramesFromPatterns(const std::vector<std::string>& patterns, int count)
{
    Vector<SpriteFrame*> frames;
    char filename[512];

    for (const auto& pat : patterns)
    {
        frames.clear();
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
                // fallback to SpriteFrameCache by base name
                char basename[128];
                // 试图从路径中提取 basename 模式（最后一段格式如 Name_%04d.png）
                // 由于 pattern 中通常包含 %04d，构造简单 basename 推测
                // 例如 "IronLance_Move_%04d.png"
                // 这里我们尝试直接用格式化后的最后部分
                // 先尝试用默认 basename 同样编号
                // 作为简单方案，尝试常见 basename：IronLance_Move / IronLance_Die
                // 具体日志利于调试
                GAME_LOG("tryLoadFramesFromPatterns - failed to create sprite from file: %s", fn.c_str());
            }
        }

        if (!frames.empty())
        {
            GAME_LOG("tryLoadFramesFromPatterns - loaded %zu frames using pattern '%s'", frames.size(), pat.c_str());
            break;
        }
    }

    // 如果仍为空，尝试直接从 SpriteFrameCache 用常见 basename 作为最后手段
    if (frames.empty())
    {
        // 尝试常见基名组合
        std::vector<std::string> basenames = {
            "IronLance_Move_%04d.png",
            "IronLance_Die_%04d.png"
        };
        for (const auto& bpat : basenames)
        {
            frames.clear();
            for (int i = 1; i <= count; ++i)
            {
                snprintf(filename, sizeof(filename), bpat.c_str(), i);
                auto frame = SpriteFrameCache::getInstance()->getSpriteFrameByName(std::string(filename));
                if (frame) frames.pushBack(frame);
                else break;
            }
            if ((int)frames.size() == count) {
                GAME_LOG("tryLoadFramesFromPatterns - loaded %d frames from SpriteFrameCache using basename '%s'", count, bpat.c_str());
                break;
            }
            frames.clear();
        }
    }

    return frames;
}

IronLance::IronLance()
    : _moveAnimation(nullptr)
    , _dieAnimation(nullptr)
    , _roomBounds(Rect::ZERO)
    , _hasRoomBounds(false)
{
}

IronLance::~IronLance()
{
    CC_SAFE_RELEASE(_moveAnimation);
    CC_SAFE_RELEASE(_dieAnimation);
}

bool IronLance::init()
{
    if (!Enemy::init()) return false;

    setEnemyType(EnemyType::MELEE);

    // 基础属性
    setMaxHP(15);
    setHP(15);

    setAttack(0);
    setMoveSpeed(60.0f);

    // 不索敌、不攻击
    setSightRange(0.0f);
    setAttackRange(0.0f);
    setAttackCooldown(10.0f);

    // 尝试分别加载 Move 和 Die 动画帧（更健壮地尝试多个路径）
    std::vector<std::string> movePatterns = {
        "Enemy/XinXing&&Iron Lance/Iron Lance/Iron Lance_Move/IronLance_Move_%04d.png",
        "Enemy/XinXing&&IronLance_Move/IronLance_Move_%04d.png",
        "Enemy/IronLance/IronLance_Move_%04d.png",
        "Enemy/IronLance_Move/IronLance_Move_%04d.png"
    };
    Vector<SpriteFrame*> moveFrames = tryLoadFramesFromPatterns(movePatterns, 8);

    std::vector<std::string> diePatterns = {
        "Enemy/XinXing&&Iron Lance/Iron Lance/Iron Lance_Die/IronLance_Die_%04d.png",
        "Enemy/XinXing&&IronLance_Die/IronLance_Die_%04d.png",
        "Enemy/IronLance/IronLance_Die_%04d.png",
        "Enemy/IronLance_Die/IronLance_Die_%04d.png"
    };
    Vector<SpriteFrame*> dieFrames = tryLoadFramesFromPatterns(diePatterns, 5);

    if (!moveFrames.empty())
    {
        _moveAnimation = Animation::createWithSpriteFrames(moveFrames, 0.12f);
        _moveAnimation->retain();
    }

    if (!dieFrames.empty())
    {
        _dieAnimation = Animation::createWithSpriteFrames(dieFrames, 0.12f);
        _dieAnimation->retain();
    }

    // 初始精灵：优先使用 Move 的第一帧；若无 Move 则使用 Die 的第一帧；否则兜底简单精灵
    if (_moveAnimation)
    {
        auto frames = _moveAnimation->getFrames();
        if (!frames.empty())
        {
            auto sprite = Sprite::createWithSpriteFrame(frames.front()->getSpriteFrame());
            this->bindSprite(sprite);
        }
    }
    else if (_dieAnimation)
    {
        auto frames = _dieAnimation->getFrames();
        if (!frames.empty())
        {
            auto sprite = Sprite::createWithSpriteFrame(frames.front()->getSpriteFrame());
            this->bindSprite(sprite);
        }
    }
    else
    {
        // 兜底：简易小点精灵
        auto sprite = Sprite::create();
        sprite->setTextureRect(Rect(0,0,16,16));
        sprite->setColor(Color3B::GRAY);
        this->bindSprite(sprite);
    }

    // 初始巡逻目标等交给 Enemy::init 的默认巡逻逻辑
    GAME_LOG("IronLance created (HP=45, single-damage per hit)");
    return true;
}

void IronLance::update(float dt)
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

void IronLance::executeAI(Player* , float dt)
{
    // 只巡逻/漂移，不索敌、不攻击
    patrol(dt);

    Vec2 dir = _patrolTarget - this->getPosition();
    if (dir.lengthSquared() > 1.0f)
    {
        move(dir, dt);
    }
    else
    {
        move(Vec2::ZERO, dt);
    }

    // 移动动画由 move() 管理（遵循 Ayao 的逻辑），因此不在此处强制启动动画。
}

void IronLance::move(const Vec2& direction, float dt)
{
    // IronLance 不会攻击，但仍需在死亡时停止移动
    if (_currentState == EntityState::DIE)
    {
        Character::move(Vec2::ZERO, dt);
        return;
    }

    const float STOP_THRESHOLD_SQ = 1.0f;
    if (direction.lengthSquared() <= STOP_THRESHOLD_SQ)
    {
        // 停止移动动作（如果存在）
        if (_sprite)
        {
            auto act = _sprite->getActionByTag(IRONL_MOVE_ACTION_TAG);
            if (act)
            {
                _sprite->stopAction(act);
            }

            // 恢复到移动动画的第一帧（通常为站立/闭嘴状态）
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

        // 调用基类移动以保持速度/状态一致
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
        if (!_sprite->getActionByTag(IRONL_MOVE_ACTION_TAG))
        {
            auto animate = Animate::create(_moveAnimation);
            auto repeat = RepeatForever::create(animate);
            repeat->setTag(IRONL_MOVE_ACTION_TAG);
            _sprite->runAction(repeat);
        }
    }
}

void IronLance::takeDamage(int damage)
{
    if (!_isAlive || damage <= 0) return;

    // 强制每次只算 1 点伤害。
    // 直接调用 GameEntity::takeDamageReported 绕过 Enemy::takeDamageReported 中的 Cup 分担逻辑，
    // 确保 IronLance 自身确实减少 HP（期望 45 次后死亡）。
    GameEntity::takeDamageReported(1);
}

int IronLance::takeDamageReported(int damage)
{
    // 坚持“所有伤害都只算 1 点”的设计
    // 保留受击无敌与基类闪烁/死亡逻辑，同时绕过 Enemy::takeDamageReported 的 Cup 分担逻辑
    if (!_isAlive || damage <= 0) return 0;

    // 与 GameEntity::takeDamageReported 一致的短暂无敌检测（避免在无敌期间重复受击）
    if (_hitInvulTimer > 0.0f) return 0;

    // 强制只造成 1 点实际伤害（调用 GameEntity 的实现以获得闪烁/无敌/死亡处理）
    return GameEntity::takeDamageReported(1);
}

void IronLance::die()
{
    // 先调用 Enemy::die() 以处理红色标记 / KongKaZi 生成功能（不改变实体状态）
    Enemy::die();

    // 设置为死亡状态并停止动作
    setState(EntityState::DIE);
    this->stopAllActions();
    if (_sprite)
    {
        _sprite->stopAllActions();
        _sprite->setVisible(true);
        _sprite->setOpacity(255);
    }

    // 如果存在可用的死亡动画帧（优先使用 _dieAnimation），把它播放为一次性视觉，然后移除节点
    Animation* deathAnim = _dieAnimation ? _dieAnimation : _moveAnimation;

    if (_sprite && deathAnim)
    {
        // 停止循环移动动画
        auto moveAct = _sprite->getActionByTag(IRONL_MOVE_ACTION_TAG);
        if (moveAct) _sprite->stopAction(moveAct);

        // 播放一次死亡动画（若只有 Move 帧也能适用）
        auto animate = Animate::create(deathAnim);
        // 结尾淡出并删除节点
        auto fadeOut = FadeOut::create(0.5f);
        auto removeCallback = CallFunc::create([this]() {
            if (this->getParent()) this->removeFromParent();
        });
        auto seq = Sequence::create(animate, fadeOut, removeCallback, nullptr);
        _sprite->runAction(seq);
    }
    else
    {
        // 使用基类的死亡效果并在结束后移除节点
        Character::die(); // 会调用 showDeathEffect()
        auto removeCallback = CallFunc::create([this]() {
            if (this->getParent()) this->removeFromParent();
        });
        // showDeathEffect 的动画大约 0.5s，延迟稍长以确保视觉完成
        this->runAction(Sequence::create(DelayTime::create(0.55f), removeCallback, nullptr));
    }
}