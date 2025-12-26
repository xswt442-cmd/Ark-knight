#ifndef __IRONLIGHTCUP_H__
#define __IRONLIGHTCUP_H__

#include "Enemy.h"
#include "cocos2d.h"

class Player;

class IronLightCup : public Enemy {
public:
    IronLightCup();
    virtual ~IronLightCup();

    virtual bool init() override;
    virtual void update(float dt) override;

    CREATE_FUNC(IronLightCup);

    // 不会索敌/攻击，随意移动
    virtual void executeAI(Player* player, float dt) override;
    virtual void attack() override { /* 不攻击 */ }

    // 每次只计作 1 次伤害，需要 35 次击中才死亡
    virtual void takeDamage(int damage) override;

    // ---------- 新增：确保 takeDamageReported 也只造成 1 点 ----------
    virtual int takeDamageReported(int damage) override;
    // -----------------------------------------------------------------

    virtual void die() override;

    virtual void setRoomBounds(const cocos2d::Rect& bounds) override { _roomBounds = bounds; _hasRoomBounds = true; }

protected:
    void loadAnimations();

    cocos2d::Animation* _moveAnimation;
    cocos2d::Animation* _dieAnimation;

    cocos2d::Rect _roomBounds;
    bool _hasRoomBounds;

    // 简单巡逻数据
    float _patrolTimer;
    float _patrolInterval;
    cocos2d::Vec2 _patrolDirection;
};

#endif // __IRONLIGHTCUP_H__