#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "Entities/Base/Character.h"

/**
 * 玩家基类
 * 继承自Character，增加输入处理、技能系统、冲刺等玩家特有功能
 * Mage、Warrior、Alchemist的基类
 */
class Player : public Character {
public:
    Player();
    virtual ~Player();
    
    virtual bool init() override;
    virtual void update(float dt) override;
    
    // ==================== 输入处理 ====================
    /**
     * 注册输入事件监听
     */
    void registerInputEvents();
    
    /**
     * 移除输入事件监听
     */
    void removeInputEvents();
    
    /**
     * 处理移动输入
     */
    void handleMoveInput(float dt);
    
    // ==================== 技能系统 ====================
    /**
     * 使用技能(纯虚函数，不同职业实现不同技能)
     */
    virtual void useSkill() = 0;
    
    /**
     * 检查技能冷却
     */
    bool canUseSkill() const;
    
    /**
     * 重置技能冷却
     */
    void resetSkillCooldown();

    /**
     * 获取技能剩余冷却时间（秒），未冷却返回0
     */
    float getSkillCooldownRemaining() const;
    
    /**
     * 获取技能总冷却时长（秒）
     */
    float getSkillCooldown() const { return _skillCooldown; }
    
    /**
     * 获取技能MP消耗
     */
    virtual int getSkillMPCost() const = 0;
    
    // ==================== 冲刺系统 ====================
    /**
     * 执行冲刺
     */
    void dash();
    
    /**
     * 检查是否可以冲刺
     */
    bool canDash() const;
    
    // ==================== 攻击实现 ====================
    void attack() override;
    
    // ==================== 属性获取 ====================
    /**
     * 获取护甲值
     */
    int getArmor() const { return _armor; }
    
    /**
     * 设置护甲值
     */
    void setArmor(int armor) { _armor = armor; }
    
    /**
     * 扣血时考虑护甲
     */
    void takeDamage(int damage) override;
    
protected:
    // 输入状态
    bool _keyW;
    bool _keyA;
    bool _keyS;
    bool _keyD;
    bool _keySpace;  // 冲刺
    
    // 技能相关
    float _skillCooldown;
    float _skillCooldownTimer;
    
    // 冲刺相关
    float _dashCooldown;
    float _dashCooldownTimer;
    bool _isDashing;
    
    // 护甲
    int _armor;
    int _maxArmor;
    
    // 输入监听器
    EventListenerKeyboard* _keyboardListener;
    EventListenerMouse* _mouseListener;
};

#endif // __PLAYER_H__
