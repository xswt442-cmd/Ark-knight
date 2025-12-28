#ifndef __DU_H__
#define __DU_H__

#include "Entities/Enemy/Enemy.h"
#include "cocos2d.h"

// 前向声明
class Player;

class Du : public Enemy {
public:
    Du();
    virtual ~Du();

    virtual bool init() override;
    CREATE_FUNC(Du);

    virtual void update(float dt) override;
    virtual void executeAI(Player* player, float dt) override;
    virtual void attack() override;
    virtual void playAttackAnimation() override;
    virtual void die() override;

    // 覆写以接收房间边界
    virtual void setRoomBounds(const cocos2d::Rect& bounds) override;

    // 覆写移动以控制动画并在发射时停止移动
    virtual void move(const cocos2d::Vec2& direction, float dt) override;

protected:
    void setupAttributes();
    void loadAnimations();

    // 发射子弹（锁定目标位置并进入发射等待状态）
    void fireBullet(Player* target);

    // 动画资源
    cocos2d::Animation* _moveAnimation;
    cocos2d::Animation* _attackAnimation;
    cocos2d::Animation* _dieAnimation;
    cocos2d::Animation* _bulletAnimation;

    // 房间边界
    cocos2d::Rect _roomBounds;
    bool _hasRoomBounds;

    // 发射状态（发射期间不移动），以及当前子弹节点
    bool _isFiring;
    cocos2d::Node* _currentBullet;

    // 持有的攻击目标（用于 windup 延迟内保持目标引用）
    Player* _attackTarget;
};

#endif // __DU_H__