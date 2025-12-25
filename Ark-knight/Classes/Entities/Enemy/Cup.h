#ifndef __CUP_H__
#define __CUP_H__

#include "Enemy.h"
#include "cocos2d.h"
#include <vector>

class Cup : public Enemy {
public:
    Cup();
    virtual ~Cup();

    virtual bool init() override;
    CREATE_FUNC(Cup);

    // Cup 不进行攻击
    virtual void attack() override {}

    // update 保持巡逻/绘制行为
    virtual void update(float dt) override;

    // 覆写 AI：不索敌、不主动靠近玩家
    virtual void executeAI(Player* player, float dt) override;

    // Cup 伤害分担接口
    void absorbDamage(int damage);

    virtual void die() override;
    virtual bool canSpawnKongKaZiOnDeath() const override { return false; }

    virtual void setRoomBounds(const cocos2d::Rect& bounds) override;

    static const std::vector<Cup*>& getInstances();

    float getShareRadius() const { return _shareRadius; }
    float getShareRatio() const { return _shareRatio; }

protected:
    void loadAnimations();

    cocos2d::Animation* _idleAnimation;
    cocos2d::Animation* _dieAnimation;
    cocos2d::Sprite* _sprite;

    cocos2d::DrawNode* _rangeIndicator;

    float _shareRadius;
    float _shareRatio;

    float _patrolTimer;
    float _patrolInterval;
    cocos2d::Vec2 _patrolDirection;

    cocos2d::Rect _roomBounds;
    bool _hasRoomBounds;

    static std::vector<Cup*> _instances;
};

#endif // __CUP_H__