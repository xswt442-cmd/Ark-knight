#include "DeYi.h"
#include "Entities/Player/Player.h"
#include "Scenes/GameScene.h"
#include "UI/FloatingText.h"
#include "cocos2d.h"

USING_NS_CC;

static const int DEYI_MOVE_ACTION_TAG = 0xD001;
static const int DEYI_DIE_ACTION_TAG  = 0xD002;

static const int DEYI_EXPLOSION_DAMAGE = 200; // 爆炸造成的“大量伤害”，可根据需要调整
static const float DEYI_EXPLOSION_RADIUS = 80.0f; // 爆炸半径（和判定距离）

DeYi::DeYi()
    : _moveAnimation(nullptr)
    , _dieAnimation(nullptr)
    , _roomBounds(Rect::ZERO)
    , _hasRoomBounds(false)
    , _hasExploded(false)
{
}

DeYi::~DeYi()
{
    CC_SAFE_RELEASE(_moveAnimation);
    CC_SAFE_RELEASE(_dieAnimation);
}

bool DeYi::init()
{
    if (!Enemy::init())
    {
        return false;
    }

    // 地面近战单位
    setEnemyType(EnemyType::MELEE);

    setupDeYiAttributes();
    loadAnimations();

    // 使用移动动画第一帧作为初始精灵
    if (_moveAnimation)
    {
        auto frames = _moveAnimation->getFrames();
        if (!frames.empty())
        {
            auto sprite = Sprite::createWithSpriteFrame(frames.front()->getSpriteFrame());
            this->bindSprite(sprite);
        }
    }

    GAME_LOG("DeYi enemy created");
    return true;
}

void DeYi::setRoomBounds(const cocos2d::Rect& bounds)
{
    _roomBounds = bounds;
    _hasRoomBounds = true;
}

void DeYi::setupDeYiAttributes()
{
    // 基础属性（根据需要调整）
    setMaxHP(120);
    setHP(120);
    setAttack(0); // 不使用常规近战攻击
    setMoveSpeed(110.0f);

    // AI 参数
    setSightRange(250.0f);
    setAttackRange(40.0f); // 接近到该范围即触发自爆

    // 攻击/前摇冷却不适用于 DeYi，但仍设置安全值
    setAttackCooldown(1.0f);
    setAttackWindup(0.1f);
}

void DeYi::loadAnimations()
{
    // 加载移动动画（DeYi_Move）
    Vector<SpriteFrame*> moveFrames;
    for (int i = 1; i <= 5; i++)
    {
        char filename[256];
        sprintf(filename, "Enemy/DeYi/DeYi_Move/DeYi_Move_%04d.png", i);
        auto sprite = Sprite::create(filename);
        if (sprite)
        {
            moveFrames.pushBack(sprite->getSpriteFrame());
        }
    }
    if (!moveFrames.empty())
    {
        _moveAnimation = Animation::createWithSpriteFrames(moveFrames, 0.12f);
        _moveAnimation->retain();
    }

    // 加载死亡动画（DeYi_Die）
    Vector<SpriteFrame*> dieFrames;
    for (int i = 1; i <= 5; i++)
    {
        char filename[256];
        sprintf(filename, "Enemy/DeYi/DeYi_Die/DeYi_Die_%04d.png", i);
        auto sprite = Sprite::create(filename);
        if (sprite)
        {
            dieFrames.pushBack(sprite->getSpriteFrame());
        }
    }
    if (!dieFrames.empty())
    {
        _dieAnimation = Animation::createWithSpriteFrames(dieFrames, 0.12f);
        _dieAnimation->retain();
    }
}

void DeYi::update(float dt)
{
    if (_currentState == EntityState::DIE)
    {
        return;
    }

    Enemy::update(dt);

    // 限制在房间边界内（若设置了）
    if (_hasRoomBounds)
    {
        Vec2 pos = this->getPosition();
        pos.x = std::max(_roomBounds.getMinX(), std::min(pos.x, _roomBounds.getMaxX()));
        pos.y = std::max(_roomBounds.getMinY(), std::min(pos.y, _roomBounds.getMaxY()));
        this->setPosition(pos);
    }
}

void DeYi::executeAI(Player* player, float dt)
{
    // 基本同 Enemy::executeAI 的检查逻辑，但进入攻击范围时直接自爆
    if (player == nullptr || player->isDead() || _currentState == EntityState::DIE)
    {
        return;
    }

    if (isPlayerInSight(player))
    {
        _hasTarget = true;

        if (isPlayerInAttackRange(player))
        {
            // 一旦达到攻击范围立刻自爆（不作为攻击态），并死亡（且只发生一次）
            if (!_hasExploded)
            {
                // 在爆炸前面向玩家并停止移动
                move(Vec2::ZERO, dt);
                faceToPosition(player->getPosition());

                // 标记已爆炸并直接执行爆炸伤害（对玩家）
                _hasExploded = true;
                doExplosion();

                // 直接调用底层伤害处理以强制自死，绕过 Enemy::takeDamage 的 Cup 分担逻辑
                GameEntity::takeDamage(this->getHP());
            }
        }
        else
        {
            // 追击玩家
            chasePlayer(player, dt);
        }
    }
    else
    {
        _hasTarget = false;
        patrol(dt);
    }
}

void DeYi::doExplosion()
{
    // 爆炸视觉与对玩家造成伤害（只影响玩家）
    // 将视觉圈添加到敌人的父节点（通常为 gameLayer），使之显示在地板之上、实体之下（与 Cup 的范围圈一致）
    Vec2 localPos = this->getPosition();
    Node* parent = this->getParent();

    auto circle = DrawNode::create();
    circle->drawSolidCircle(Vec2::ZERO, DEYI_EXPLOSION_RADIUS, 0, 32, Color4F(1.0f, 0.4f, 0.2f, 0.6f)); // 橙色半透明

    if (parent)
    {
        int rangeZ = Constants::ZOrder::ENTITY - 1;
        parent->addChild(circle, rangeZ);
        circle->setPosition(localPos);
        circle->setGlobalZOrder(static_cast<float>(rangeZ));
    }
    else
    {
        // 退回：添加到 running scene（原行为）
        Scene* running = Director::getInstance()->getRunningScene();
        if (running)
        {
            Vec2 worldPos = this->convertToWorldSpace(Vec2::ZERO);
            running->addChild(circle, Constants::ZOrder::EFFECT);
            circle->setPosition(worldPos);
        }
    }

    // 在圆自身上运行淡出并移除
    auto fade = FadeOut::create(0.5f);
    auto removeCircle = CallFunc::create([circle]() {
        if (circle->getParent()) circle->removeFromParent();
    });
    circle->runAction(Sequence::create(DelayTime::create(0.1f), fade, removeCircle, nullptr));

    // 对玩家造成伤害（若在半径内）
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

    if (gs)
    {
        Player* player = gs->getPlayer();
        if (player && !player->isDead())
        {
            float dist = player->getPosition().distance(this->getPosition());
            if (dist <= DEYI_EXPLOSION_RADIUS)
            {
                player->takeDamage(DEYI_EXPLOSION_DAMAGE);

                // 显示伤害浮字（同毒伤表现）
                Scene* runningScene = Director::getInstance()->getRunningScene();
                if (runningScene)
                {
                    Vec2 worldP = player->convertToWorldSpace(Vec2::ZERO);
                    FloatingText::show(runningScene, worldP, std::to_string(DEYI_EXPLOSION_DAMAGE), Color3B(255,180,50));
                }
            }
        }
    }

    GAME_LOG("DeYi explosion at (%.1f, %.1f)", this->getPositionX(), this->getPositionY());
}

void DeYi::die()
{
    // 先调用基类以处理红色标记 -> KongKaZi 生成功能（如果有）
    Enemy::die();

    // 如果尚未爆炸（即被玩家击杀），在此触发一次爆炸
    if (!_hasExploded)
    {
        _hasExploded = true;
        doExplosion();
    }

    // 防止重复死亡处理
    if (_currentState == EntityState::DIE && !_isAlive)
    {
        return;
    }

    setState(EntityState::DIE);
    _isAlive = false;

    this->stopAllActions();
    if (_sprite)
    {
        _sprite->stopAllActions();
        _sprite->setVisible(true);
        _sprite->setOpacity(255);
    }

    // 播放死亡动画并移除
    if (_dieAnimation && _sprite)
    {
        auto animate = Animate::create(_dieAnimation);
        auto fadeOut = FadeOut::create(0.5f);
        auto removeCallback = CallFunc::create([this]() {
            this->removeFromParent();
        });
        auto sequence = Sequence::create(animate, fadeOut, removeCallback, nullptr);
        _sprite->runAction(sequence);
    }
    else
    {
        if (_sprite)
        {
            auto fadeOut = FadeOut::create(0.5f);
            auto removeCallback = CallFunc::create([this]() {
                this->removeFromParent();
            });
            auto sequence = Sequence::create(fadeOut, removeCallback, nullptr);
            _sprite->runAction(sequence);
        }
        else
        {
            this->removeFromParent();
        }
    }
}

void DeYi::move(const cocos2d::Vec2& direction, float dt)
{
    // 行为类似 Ayao：攻击/死亡状态不移动，控制移动动画与朝向
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
            auto act = _sprite->getActionByTag(DEYI_MOVE_ACTION_TAG);
            if (act)
            {
                _sprite->stopAction(act);
            }
            if (_moveAnimation)
            {
                auto frames = _moveAnimation->getFrames();
                if (!frames.empty())
                {
                    _sprite->setSpriteFrame(frames.front()->getSpriteFrame());
                }
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

        if (!_sprite->getActionByTag(DEYI_MOVE_ACTION_TAG))
        {
            auto animate = Animate::create(_moveAnimation);
            auto repeat = RepeatForever::create(animate);
            repeat->setTag(DEYI_MOVE_ACTION_TAG);
            _sprite->runAction(repeat);
        }
    }
}