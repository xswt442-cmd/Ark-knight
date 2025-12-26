#ifndef __ENEMY_H__
#define __ENEMY_H__

#include "Entities/Base/Character.h"
#include "cocos2d.h"
#include <vector>

// 前向声明
class Player;

/**
 * 敌人基类
 * 继承自Character，增加AI逻辑、寻路、攻击判定
 */
class Enemy : public Character {
public:
    Enemy();
    virtual ~Enemy();

    virtual bool init() override;
    virtual void update(float dt) override;

    CREATE_FUNC(Enemy);

    // ==================== AI系统 ====================
    virtual void executeAI(Player* player, float dt);
    void setEnemyType(EnemyType type) { _enemyType = type; }
    EnemyType getEnemyType() const { return _enemyType; }

    // 追击/巡逻/攻击（略）
    bool isPlayerInSight(Player* player) const;
    void chasePlayer(Player* player, float dt);
    void patrol(float dt);
    void attack() override;
    bool isPlayerInAttackRange(Player* player) const;
    virtual void attackPlayer(Player* player);
    virtual void playAttackAnimation();

    // 属性设置（略）
    void setSightRange(float range) { _sightRange = range; }
    float getSightRange() const { return _sightRange; }
    void setAttackRange(float range) { _attackRange = range; }
    float getAttackRange() const { return _attackRange; }
    void setAttackWindup(float seconds) { _attackWindup = seconds; }
    float getAttackWindup() const { return _attackWindup; }

    // 新增：是否算作房间清除计数（默认 true）
    virtual bool countsForRoomClear() const { return true; }

    // ==================== Nymph 毒伤系统接口（已存在） ====================
    void applyNymphPoison(int sourceAttack);
    int getPoisonStacks() const { return _poisonStacks; }

    // ==================== Stealth（隐身） 管理 ====================
    /**
     * 将一个隐身“来源”注册到该敌人（来源可以是烟雾 DrawNode 或其地址）
     * 多个来源可共存；只有当来源计数从 0 => 1 时才真正设置颜色/隐身标记
     */
    void addStealthSource(void* source);
    /**
     * 移除之前注册的隐身来源；当计数归零时取消隐身并恢复颜色（若有毒则恢复毒色）
     */
    void removeStealthSource(void* source);
    /**
     * 是否当前处于隐身（至少存在一个来源）
     */
    bool isStealthed() const { return !_stealthSources.empty(); }

    // ==================== 红色标记 / KongKaZi 生成功能 ====================
    /**
     * 在敌人生成后（GameScene 等处调用）尝试以概率 chance 使其泛红并获得死亡爆炸生成恐卡兹能力
     * 注意：该方法会忽略不能生成恐卡兹的敌人（例如 KongKaZi 自身）
     */
    void tryApplyRedMark(float chance);

    /**
     * 子类可覆盖以禁止自己死后生成恐卡兹（KongKaZi 应覆盖为 false）
     */
    virtual bool canSpawnKongKaZiOnDeath() const { return true; }

    /**
     * 在敌人死亡时执行（包括生成恐卡兹逻辑），覆盖自 Character::die
     * 子类若覆写 die() 请确保在适当时机调用 Enemy::die() 以触发该通用逻辑。
     */
    virtual void die() override;

    /**
     * 覆写 takeDamage 以支持 Cup 的伤害分担逻辑（非 Cup 的敌人受伤时会检查附近 Cup）
     */
    virtual void takeDamage(int damage) override;

    /**
     * 与 takeDamage 对应的“有返回值”的版本：返回实际对该敌人造成的 HP 减少值（可用于精确显示浮动伤害）
     */
    virtual int takeDamageReported(int damage);

    /**
     * 设置房间边界（默认空实现），子类可覆写以接收房间边界
     */
    virtual void setRoomBounds(const cocos2d::Rect& bounds);

    EnemyType _enemyType;
    // 敌人类型

    float _sightRange;
    // 视野范围
    float _attackRange;
    // 攻击范围

    // 巡逻相关
    cocos2d::Vec2 _patrolTarget;
    // 巡逻目标点
    float _patrolTimer;
    // 巡逻计时器
    float _patrolInterval;
    // 巡逻间隔

    // AI状态
    bool _hasTarget;
    // 是否有目标

    // 攻击前摇（windup）时长（秒），默认 0.5f
    float _attackWindup;
    // ========== Nymph 中毒状态 ==========
    int _poisonStacks;
    float _poisonTimer;
    float _poisonTickAcc;
    int _poisonSourceAttack;
    cocos2d::Color3B _poisonOriginalColor;
    bool _poisonColorSaved;
    static const int POISON_MAX_STACKS = 100;
    static constexpr float POISON_DURATION = 10.0f;
    static constexpr float POISON_TICK_INTERVAL = 0.5f;
    static constexpr float POISON_TICK_RATIO = 0.1f;
    // 每层每次造成源攻击 10%

    // ========== Stealth 源列表（支持多来源） ==========
    std::vector<void*> _stealthSources;
    cocos2d::Color3B _stealthOriginalColor;
    bool _stealthColorSaved;

    // ========== 红色标记 ==========
    bool _isRedMarked;
};

#endif // __ENEMY_H__
