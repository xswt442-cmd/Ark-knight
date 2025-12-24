#ifndef __IRONLANCE_H__
#define __IRONLANCE_H__

#include "Enemy.h"
#include "cocos2d.h"

///
class IronLance : public Enemy {
public:
    IronLance();
    virtual ~IronLance();

    virtual bool init() override;
    virtual void update(float dt) override;

    CREATE_FUNC(IronLance);

    // 现在算作房间清除计数（改为 true）
    virtual bool countsForRoomClear() const override { return true; }

    // 仅移动（覆写 AI）
    virtual void executeAI(Player* player, float dt) override;

    // 任意一次受伤仅扣 1 点
    virtual void takeDamage(int damage) override;

    // 覆写 die()，确保死亡时执行视觉播放 + 清理
    virtual void die() override;

    // 房间边界
    virtual void setRoomBounds(const cocos2d::Rect& bounds) override { _roomBounds = bounds; _hasRoomBounds = true; }



protected:

    cocos2d::Animation* _moveAnimation;
    cocos2d::Rect _roomBounds;
    bool _hasRoomBounds;
};

#endif // __IRONLANCE_H__