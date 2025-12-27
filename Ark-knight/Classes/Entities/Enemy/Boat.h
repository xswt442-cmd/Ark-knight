#ifndef __BOAT_H__
#define __BOAT_H__

#include "Entities/Enemy/Enemy.h"

class Boat : public Enemy {
public:
    Boat();
    virtual ~Boat();

    virtual bool init() override;
    virtual void update(float dt) override;
    
    // 覆写移动：忽略障碍物碰撞，但保留边界限制
    virtual void move(const cocos2d::Vec2& direction, float dt) override;

    CREATE_FUNC(Boat);

    // 设置房间边界
    virtual void setRoomBounds(const cocos2d::Rect& bounds) override;

    // 托生莲座不计入房间清理计数（因为它会消失）
    virtual bool countsForRoomClear() const override { return false; }

    // 强制消失（Boss技能结束时调用）
    void forceDissipate();

protected:
    void loadAnimations();
    void pickNewDirection();
    void checkPlayerCollision();

    // 状态
    bool _isMoving;
    float _idleTimer;      // 出场待机计时
    float _lifeTimer;      // 存活计时（虽然由Boss控制，但自身也维护一个以防万一）
    int _collisionCount;   // 碰撞次数
    int _absorbedCount;    // 吸收计数器（题目要求存入自身）
    
    cocos2d::Rect _roomBounds;
    cocos2d::Vec2 _currentMoveDir;

    // 动画
    cocos2d::Animation* _animIdle;
    cocos2d::Animation* _animMove;
    cocos2d::Animation* _animDie;

    // 标记
    static const int BOAT_ACTION_TAG = 0xB0A7;
};

#endif // __BOAT_H__