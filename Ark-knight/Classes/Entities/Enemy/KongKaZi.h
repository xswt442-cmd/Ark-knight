#ifndef __KONGKAZI_H__
#define __KONGKAZI_H__

#include "Enemy.h"

/**
 * 恐卡兹 - 由被标记的敌人死亡时生成
 * 设计：与 Ayao 完全相同的实现方式（动画、移动、攻击、死亡）
 */
class KongKaZi : public Enemy {
public:
    KongKaZi();
    virtual ~KongKaZi();

    virtual bool init() override;
    CREATE_FUNC(KongKaZi);

    virtual void update(float dt) override;
    void setupKongKaZiAttributes();
    void loadAnimations();

    void attack() override;
    void die() override;

    virtual void move(const cocos2d::Vec2& direction, float dt) override;
    virtual void playAttackAnimation() override;

    // 覆盖以阻止从自身生成恐卡兹
    virtual bool canSpawnKongKaZiOnDeath() const override { return false; }

    void setRoomBounds(const cocos2d::Rect& bounds) { _roomBounds = bounds; _hasRoomBounds = true; }

protected:
    Animation* _moveAnimation;
    Animation* _attackAnimation;
    Animation* _dieAnimation;

    cocos2d::Rect _roomBounds;
    bool _hasRoomBounds;
};

#endif // __KONGKAZI_H__