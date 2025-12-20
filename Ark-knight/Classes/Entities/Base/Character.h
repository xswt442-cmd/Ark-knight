#ifndef __CHARACTER_H__
#define __CHARACTER_H__

#include "GameEntity.h"

/**
 * 角色基类
 * 继承自GameEntity，增加移动、状态机、攻击等功能
 * 玩家和敌人的共同基类
 */
class Character : public GameEntity {
public:
    Character();
    virtual ~Character();
    
    virtual bool init() override;
    virtual void update(float dt) override;
    
    // ==================== 状态管理 ====================
    /**
     * 获取当前状态
     */
    EntityState getState() const { return _currentState; }
    
    /**
     * 设置状态
     */
    virtual void setState(EntityState state);
    
    /**
     * 状态机更新
     */
    virtual void updateStateMachine(float dt);
    
    // ==================== 移动相关 ====================
    /**
     * 获取移动速度
     */
    float getMoveSpeed() const { return _moveSpeed; }
    
    /**
     * 设置移动速度
     */
    void setMoveSpeed(float speed) { _moveSpeed = speed; }
    
    /**
     * 移动到指定方向
     * @param direction 移动方向(归一化向量)
     */
    virtual void move(const Vec2& direction, float dt);
    
    /**
     * 面向指定位置
     */
    void faceToPosition(const Vec2& targetPos);
    
    // ==================== 攻击相关 ====================
    /**
     * 攻击(纯虚函数，子类必须实现)
     */
    virtual void attack() = 0;
    
    /**
     * 获取攻击力
     */
    int getAttack() const { return _attack; }
    
    /**
     * 设置攻击力
     */
    void setAttack(int attack) { _attack = attack; }
    
    /**
     * 检查攻击冷却
     */
    bool canAttack() const;
    
    /**
     * 重置攻击冷却
     */
    void resetAttackCooldown();
    
    // ==================== 法力值管理(可选) ====================
    /**
     * 获取法力值
     */
    int getMP() const { return _mp; }
    
    /**
     * 设置法力值
     */
    void setMP(int mp) { _mp = mp; }
    
    /**
     * 获取最大法力值
     */
    int getMaxMP() const { return _maxMP; }
    
    /**
     * 设置最大法力值
     */
    void setMaxMP(int maxMP) { _maxMP = maxMP; }
    
    /**
     * 消耗法力值
     */
    bool consumeMP(int cost);
    
    // ==================== 死亡处理 ====================
    void die() override;
    
protected:
    EntityState _currentState;     // 当前状态
    
    float _moveSpeed;              // 移动速度
    int _attack;                   // 攻击力
    
    float _attackCooldown;         // 攻击冷却时间
    float _attackCooldownTimer;    // 攻击冷却计时器
    
    int _mp;                       // 当前法力值
    int _maxMP;                    // 最大法力值
    
    Vec2 _facingDirection;         // 面朝方向
};

#endif // __CHARACTER_H__
