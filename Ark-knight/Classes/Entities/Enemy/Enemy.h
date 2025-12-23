#ifndef __ENEMY_H__
#define __ENEMY_H__

#include "Entities/Base/Character.h"

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
    /**
     * 执行AI逻辑
     * @param player 目标玩家
     */
    virtual void executeAI(Player* player, float dt);
    
    /**
     * 设置敌人类型
     */
    void setEnemyType(EnemyType type) { _enemyType = type; }
    
    /**
     * 获取敌人类型
     */
    EnemyType getEnemyType() const { return _enemyType; }
    
    // ==================== 追击系统 ====================
    /**
     * 检测玩家是否在视野范围内
     */
    bool isPlayerInSight(Player* player) const;
    
    /**
     * 追击玩家
     */
    void chasePlayer(Player* player, float dt);
    
    /**
     * 巡逻
     */
    void patrol(float dt);
    
    // ==================== 攻击系统 ====================
    /**
     * 攻击实现（发起攻击，播放前摇动画等）
     * Enemy::attack 会被子类覆盖以播放具体前摇/视觉
     */
    void attack() override;
    
    /**
     * 检测玩家是否在攻击范围内
     */
    bool isPlayerInAttackRange(Player* player) const;
    
    /**
     * 攻击玩家（造成伤害）
     */
    virtual void attackPlayer(Player* player);
    
    /**
     * 播放命中/伤害时的视觉动画（子类覆盖）
     * 调用时机：在风箱结束并确认命中前调用，保证每次真正造成伤害都会有动画反馈
     */
    virtual void playAttackAnimation();
    
    // ==================== 属性设置 ====================
    /**
     * 设置视野范围
     */
    void setSightRange(float range) { _sightRange = range; }
    
    /**
     * 获取视野范围
     */
    float getSightRange() const { return _sightRange; }
    
    /**
     * 设置攻击范围
     */
    void setAttackRange(float range) { _attackRange = range; }
    
    /**
     * 获取攻击范围
     */
    float getAttackRange() const { return _attackRange; }

    /**
     * 设置攻击前摇时长（秒），AI windup 将以此时长判断玩家是否逃离
     */
    void setAttackWindup(float seconds) { _attackWindup = seconds; }

    /**
     * 获取攻击前摇时长（秒）
     */
    float getAttackWindup() const { return _attackWindup; }

    // ==================== Nymph 毒伤系统接口 ====================
    /**
     * 被 Nymph 攻击时调用：重置毒倒计时（10s），并叠加一层（最多100层）。
     * @param sourceAttack Nymph 当时的攻击力（用于毒伤与技能加成计算）
     */
    void applyNymphPoison(int sourceAttack);

    /**
     * 获取当前毒层数
     */
    int getPoisonStacks() const { return _poisonStacks; }

protected:
    EnemyType _enemyType;           // 敌人类型
    
    float _sightRange;              // 视野范围
    float _attackRange;             // 攻击范围
    
    // 巡逻相关
    Vec2 _patrolTarget;             // 巡逻目标点
    float _patrolTimer;             // 巡逻计时器
    float _patrolInterval;          // 巡逻间隔
    
    // AI状态
    bool _hasTarget;                // 是否有目标

    // 攻击前摇（windup）时长（秒），默认 0.5f
    float _attackWindup;


    // ========== Nymph 中毒状态 ==========
    int _poisonStacks;              // 当前毒层数
    float _poisonTimer;             // 剩余毒持续时间（秒），重置为 10.0f
    float _poisonTickAcc;           // 用于 0.5s 一次的计时
    int _poisonSourceAttack;        // 记录造成毒的 Nymph 当时的攻击力（用于计算每次毒伤 10%）
    Color3B _poisonOriginalColor;   // 记录被毒前的精灵颜色（解除毒时恢复）
    bool _poisonColorSaved;         // 是否已保存原始颜色
    static const int POISON_MAX_STACKS = 100;
    static constexpr float POISON_DURATION = 10.0f;
    static constexpr float POISON_TICK_INTERVAL = 0.5f;
    static constexpr float POISON_TICK_RATIO = 0.1f; // 每层每次 tick 造成源攻击 10%
};

#endif // __ENEMY_H__
