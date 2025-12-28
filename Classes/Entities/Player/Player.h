#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "Entities/Base/Character.h"

// 玩家基类
class Player : public Character {
public:
    Player();
    virtual ~Player();
    
    virtual bool init() override;
    virtual void update(float dt) override;
    
    // 输入处理
    // 注册输入事件监听
    void registerInputEvents();
    
    // 移除输入事件监听
    void removeInputEvents();
    
    // 处理移动输入
    void handleMoveInput(float dt);
    
    // 技能系统
    // 使用技能(纯虚函数，不同职业实现不同技能)
    virtual void useSkill() = 0;
    
    // 检查技能冷却
    bool canUseSkill() const;
    
    // 重置技能冷却
    void resetSkillCooldown();

    // 获取技能剩余冷却时间（秒），未冷却返回0
    float getSkillCooldownRemaining() const;
    
    // 获取技能总冷却时长（秒）
    float getSkillCooldown() const { return _skillCooldown; }
    
    // 获取技能MP消耗
    virtual int getSkillMPCost() const = 0;
    
    // 治疗技能
    // 使用治疗术（所有角色通用）
    void useHeal();
    
    // 检查是否可以使用治疗
    bool canUseHeal() const;
    
    // 获取治疗剩余冷却时间
    float getHealCooldownRemaining() const;
    
    // 获取治疗总冷却时长
    float getHealCooldown() const { return _healCooldown; }
    
    // 冲刺系统
    // 执行冲刺
    void dash();
    
    // 检查是否可以冲刺
    bool canDash() const;
    
    // 攻击实现
    void attack() override;
    
    // 属性获取
    // 获取护甲值
    int getArmor() const { return _armor; }
    
    // 设置护甲值
    void setArmor(int armor) { _armor = armor; }

    // 道具增益接口
    void addDamageReduction(float percent);
    void multiplyAttack(float factor);
    void multiplyAttackCooldown(float factor);
    void multiplyMaxHP(float factor, float healPercent);
    void addMPRegenBonus(float bonusPerSec);
    void addHealPowerMultiplier(float delta);
    void addHPRegenPercent(float delta);
    float getAttackCooldownValue() const { return getAttackCooldown(); }
    
    // 扣血时考虑护甲
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
    
    // 治疗相关
    float _healCooldown;
    float _healCooldownTimer;
    int _healMPCost;
    int _healAmount;

    // 道具相关增益
    float _damageReductionPct;      // 总减伤百分比 0~1
    float _mpRegenBonusPerSec;      // 额外MP恢复/秒
    float _healPowerMultiplier;     // 治疗量乘算
    float _hpRegenPercentPerSec;    // 每秒按最大生命回复百分比
    float _hpRegenAccumulator;      // 血量恢复累积器
    
    // MP恢复累积器（避免浮点转int精度丢失）
    float _mpRegenAccumulator;
    
    // 冲刺相关
    float _dashCooldown;
    float _dashCooldownTimer;
    bool _isDashing;
    
    // 护甲
    int _armor;
    int _maxArmor;
    
    // 受击闪烁冷却
    float _blinkCooldown;
    float _blinkCooldownTimer;
    
    // 输入监听器
    EventListenerKeyboard* _keyboardListener;
    EventListenerMouse* _mouseListener;
};

#endif // __PLAYER_H__
