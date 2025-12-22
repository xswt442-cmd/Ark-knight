#ifndef __AYAO_H__
#define __AYAO_H__

#include "Enemy.h"

/**
 * 阿咬 - 基础近战敌人
 * 特点：血量适中，会巡逻并追击玩家，近身攻击
 */
class Ayao : public Enemy {
public:
    Ayao();
    virtual ~Ayao();
    
    virtual bool init() override;
    
    CREATE_FUNC(Ayao);
    
    /**
     * 设置阿咬属性
     */
    void setupAyaoAttributes();
    
    /**
     * 加载阿咬动画
     */
    void loadAnimations();
    
protected:
    // 动画资源
    Animation* _moveAnimation;
    Animation* _attackAnimation;
    Animation* _dieAnimation;
};

#endif // __AYAO_H__
