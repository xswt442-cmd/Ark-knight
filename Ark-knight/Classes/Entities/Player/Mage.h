#ifndef __MAGE_H__
#define __MAGE_H__

#include "Player.h"

/**
 * 妮芙(Nymph) - 法师职业
 * 特点：远程攻击、高法力值、技能为强化状态
 * 
 * 属性：100HP, 150MP
 * 普攻：发射粉色子弹，1秒1发，造成100%攻击力伤害
 * 技能：进入强化状态8秒，子弹伤害200%，攻速提升100%（0.5秒1发）
 *       消耗80蓝，15秒CD
 */
class Mage : public Player {
public:
    Mage();
    virtual ~Mage();
    
    virtual bool init() override;
    virtual void update(float dt) override;
    
    CREATE_FUNC(Mage);
    
    // ==================== 实现抽象接口 ====================
    /**
     * 普通攻击：粉色子弹
     */
    void attack() override;
    
    /**
     * 使用技能：进入强化状态
     */
    void useSkill() override;
    
    /**
     * 获取技能MP消耗
     */
    int getSkillMPCost() const override { return 80; }
    
    /**
     * 是否处于强化状态
     */
    bool isEnhanced() const { return _isEnhanced; }
    
private:
    /**
     * 发射子弹（普攻和强化状态通用）
     */
    void shootBullet();
    
    /**
     * 进入强化状态
     */
    void enterEnhancedState();
    
    /**
     * 退出强化状态
     */
    void exitEnhancedState();
    
    // 强化状态相关
    bool _isEnhanced;           // 是否处于强化状态
    float _enhancedTimer;       // 强化状态剩余时间
    float _enhancedDuration;    // 强化状态持续时间（8秒）
    
    // 攻速相关
    float _baseAttackInterval;  // 基础攻击间隔（1秒）
    float _attackTimer;         // 攻击计时器
};

#endif // __MAGE_H__
