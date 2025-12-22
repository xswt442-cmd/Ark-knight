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
     * 更新（限制房间范围）
     */
    virtual void update(float dt) override;
    
    /**
     * 设置阿咬属性
     */
    void setupAyaoAttributes();
    
    /**
     * 加载阿咬动画
     */
    void loadAnimations();
    
    /**
     * 重写攻击方法，播放攻击动画
     */
    void attack() override;
    
    /**
     * 重写死亡方法，播放死亡动画
     */
    void die() override;
    
    /**
     * 设置房间边界（限制移动范围）
     */
    void setRoomBounds(const cocos2d::Rect& bounds) { _roomBounds = bounds; _hasRoomBounds = true; }
    
protected:
    // 动画资源
    Animation* _moveAnimation;
    Animation* _attackAnimation;
    Animation* _dieAnimation;
    
    // 房间边界
    cocos2d::Rect _roomBounds;
    bool _hasRoomBounds;
};

#endif // __AYAO_H__
