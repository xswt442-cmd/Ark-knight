#include "Ayao.h"

Ayao::Ayao()
    : _moveAnimation(nullptr)
    , _attackAnimation(nullptr)
    , _dieAnimation(nullptr)
{
}

Ayao::~Ayao()
{
    CC_SAFE_RELEASE(_moveAnimation);
    CC_SAFE_RELEASE(_attackAnimation);
    CC_SAFE_RELEASE(_dieAnimation);
}

bool Ayao::init()
{
    if (!Enemy::init())
    {
        return false;
    }
    
    // 设置敌人类型
    setEnemyType(EnemyType::MELEE);
    
    // 设置属性
    setupAyaoAttributes();
    
    // 加载动画
    loadAnimations();
    
    GAME_LOG("Ayao enemy created");
    
    return true;
}

void Ayao::setupAyaoAttributes()
{
    // 阿咬基础属性
    setMaxHP(50);
    setHP(50);
    setAttack(10);
    setMoveSpeed(100.0f);
    
    // AI参数
    setSightRange(250.0f);      // 视野范围
    setAttackRange(40.0f);      // 攻击范围
    
    // 攻击冷却
    setAttackCooldown(1.5f);    // 1.5秒攻击一次
}

void Ayao::loadAnimations()
{
    // ==================== 移动动画 ====================
    Vector<SpriteFrame*> moveFrames;
    for (int i = 1; i <= 5; i++)
    {
        char filename[128];
        sprintf(filename, "Enemy/AYao/AYao_Move/AYao_Move_%04d.png", i);
        auto frame = Sprite::create(filename)->getSpriteFrame();
        if (frame)
        {
            moveFrames.pushBack(frame);
        }
    }
    
    if (!moveFrames.empty())
    {
        _moveAnimation = Animation::createWithSpriteFrames(moveFrames, 0.1f);
        _moveAnimation->retain();
    }
    
    // ==================== 攻击动画 ====================
    Vector<SpriteFrame*> attackFrames;
    for (int i = 1; i <= 5; i++)
    {
        char filename[128];
        sprintf(filename, "Enemy/AYao/AYao_Attack/AYao_Attack_%04d.png", i);
        auto frame = Sprite::create(filename);
        if (frame)
        {
            attackFrames.pushBack(frame->getSpriteFrame());
        }
    }
    
    if (!attackFrames.empty())
    {
        _attackAnimation = Animation::createWithSpriteFrames(attackFrames, 0.1f);
        _attackAnimation->retain();
    }
    
    // ==================== 死亡动画 ====================
    Vector<SpriteFrame*> dieFrames;
    for (int i = 1; i <= 3; i++)
    {
        char filename[128];
        sprintf(filename, "Enemy/AYao/AYao_Die/AYao_Die_%04d.png", i);
        auto frame = Sprite::create(filename);
        if (frame)
        {
            dieFrames.pushBack(frame->getSpriteFrame());
        }
    }
    
    if (!dieFrames.empty())
    {
        _dieAnimation = Animation::createWithSpriteFrames(dieFrames, 0.15f);
        _dieAnimation->retain();
    }
    
    // 设置初始精灵（使用移动动画第一帧）
    if (!moveFrames.empty())
    {
        this->setSpriteFrame(moveFrames.at(0));
    }
    
    GAME_LOG("Ayao animations loaded");
}
