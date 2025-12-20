#ifndef __MAGE_H__
#define __MAGE_H__

#include "Player.h"

/**
 * 法师职业
 * 特点：远程攻击、高法力值、技能为火球术
 */
class Mage : public Player {
public:
    Mage();
    virtual ~Mage();
    
    virtual bool init() override;
    
    CREATE_FUNC(Mage);
    
    // ==================== 实现抽象接口 ====================
    /**
     * 使用技能：火球术
     */
    void useSkill() override;
    
    /**
     * 获取技能MP消耗
     */
    int getSkillMPCost() const override { return 30; }
    
private:
    /**
     * 释放火球
     */
    void castFireball();
};

#endif // __MAGE_H__
