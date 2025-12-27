#ifndef __WARRIOR_H__
#define __WARRIOR_H__

#include "Player.h"

/**
 * 泥岩(Mudrock) - 战士职业
 * 特点：近战范围攻击、高血量、抡锤造成伤害
 * 
 * 属性：150HP, 80MP
 * 普攻：抡锤对范围内敌人造成伤害
 * 技能：恢复20%生命值，抡锤范围扩大，攻击+200%
 *       消耗60蓝，18秒CD
 */
class Warrior : public Player {
public:
    Warrior();
    virtual ~Warrior();
    
    virtual bool init() override;
    virtual void update(float dt) override;
    
    CREATE_FUNC(Warrior);
    
    // ==================== 实现抽象接口 ====================
    /**
     * 普通攻击：抡锤范围攻击
     */
    void attack() override;
    
    /**
     * 使用技能：回血+强化状态
     */
    void useSkill() override;
    
    /**
     * 获取技能MP消耗
     */
    int getSkillMPCost() const override { return 60; }
    
    /**
     * 是否处于强化状态
     */
    bool isEnhanced() const { return _isEnhanced; }
    
    /**
     * 重写受伤逻辑，优先扣除护盾
     */
    virtual void takeDamage(int damage) override;

protected:
    /**
     * 重写状态切换，播放对应动画
     */
    void setState(EntityState state) override;
    
private:
    /**
     * 初始化动画
     */
    void initAnimations();
    
    /**
     * 播放指定动画
     */
    void playAnimation(const std::string& name, bool loop = true);
    
    /**
     * 执行近战范围攻击
     */
    void performMeleeAttack();
    
    /**
     * 进入强化状态
     */
    void enterEnhancedState();
    
    /**
     * 退出强化状态
     */
    void exitEnhancedState();
    
    /**
     * 增加护盾
     */
    void addShield(int amount);

    /**
     * 更新护盾条显示（会同步位置到父节点）
     */
    void updateShieldBar();

    // 强化状态相关
    bool _isEnhanced;           // 是否处于强化状态
    float _enhancedTimer;       // 强化状态剩余时间
    float _enhancedDuration;    // 强化状态持续时间
    
    // 攻击相关
    float _baseAttackInterval;  // 基础攻击间隔
    float _baseAttackRange;     // 基础攻击范围
    float _attackTimer;         // 攻击计时器
    
    // 护盾相关
    int _currentShield;         // 当前护盾值
    cocos2d::DrawNode* _shieldBarNode; // 护盾条节点（绘制）
    cocos2d::Label* _shieldLabel;      // 护盾数值文本（在护盾条下方显示）

    // 动画相关
    std::map<std::string, Animation*> _animations;  // 动画缓存
    std::string _currentAnimName;                    // 当前播放的动画名
};

#endif // __WARRIOR_H__
