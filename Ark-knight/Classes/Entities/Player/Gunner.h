#ifndef __GUNNER_H__
#define __GUNNER_H__

#include "Player.h"

/**
 * 维什戴尔(Wisdael) - 炮手职业
 * 特点：远程攻击、发射子弹造成范围伤害
 * 
 * 属性：100HP, 100MP
 * 普攻：发射子弹，造成100%攻击力伤害
 * 技能：发射巨大炸弹造成范围伤害，攻击+200%
 *       消耗80蓝，15秒CD
 */
class Gunner : public Player {
public:
    Gunner();
    virtual ~Gunner();
    
    virtual bool init() override;
    virtual void update(float dt) override;
    
    CREATE_FUNC(Gunner);
    
    // ==================== 实现抽象接口 ====================
    /**
     * 普通攻击：发射子弹
     */
    void attack() override;
    
    /**
     * 使用技能：发射巨大炸弹
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
     * 发射子弹
     */
    void shootBullet();
    
    /**
     * 发射技能炸弹
     */
    void shootSkillBomb();
    
    /**
     * 创建爆炸效果并造成范围伤害
     */
    void createExplosion(Node* parent, const Vec2& pos, int damage, float radius);
    
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
    float _enhancedDuration;    // 强化状态持续时间
    
    // 攻速相关
    float _baseAttackInterval;  // 基础攻击间隔
    float _attackTimer;         // 攻击计时器
    
    // 动画相关
    std::map<std::string, Animation*> _animations;  // 动画缓存
    std::string _currentAnimName;                    // 当前播放的动画名
};

#endif // __GUNNER_H__
