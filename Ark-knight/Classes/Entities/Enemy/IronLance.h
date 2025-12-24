#ifndef __IRONLANCE_H__
#define __IRONLANCE_H__

#include "Enemy.h"
#include "cocos2d.h"

///**
// * 铁矛头（IronLance）
// * - 只移动/漂浮，不会攻击也不会索敌
// * - HP = 45，任何一次受伤固定扣 1（覆写 takeDamage）
// * - 不计入房间清除（countsForRoomClear 返回 false）
// * - 精灵体积缩小到 0.5
// */

class IronLance : public Enemy {
public:
    IronLance();
    virtual ~IronLance();

    virtual bool init() override;
    virtual void update(float dt) override;

    CREATE_FUNC(IronLance);

    // 不计入房间清除计数
    virtual bool countsForRoomClear() const override { return false; }

    // 仅移动（覆写 AI）
    virtual void executeAI(Player* player, float dt) override;

    // 任意一次受伤仅扣 1 点
    virtual void takeDamage(int damage) override;

    // 房间边界
    virtual void setRoomBounds(const cocos2d::Rect& bounds) override { _roomBounds = bounds; _hasRoomBounds = true; }



protected:

    cocos2d::Animation* _moveAnimation;
    cocos2d::Rect _roomBounds;
    bool _hasRoomBounds;
};

#endif // __IRONLANCE_H__