#ifndef __BOAT_H__
#define __BOAT_H__

#include "Entities/Enemy/Enemy.h"

class Boat : public Enemy {
public:
    Boat();
    virtual ~Boat();

    virtual bool init() override;
    virtual void update(float dt) override;
    
    // 重写移动：遇到障碍或碰撞房间边界反弹
    virtual void move(const cocos2d::Vec2& direction, float dt) override;

    CREATE_FUNC(Boat);

    // 设置房间边界
    virtual void setRoomBounds(const cocos2d::Rect& bounds) override;

    // 托生莲座不计入房间清理计数
    virtual bool countsForRoomClear() const override { return false; }

    // 强制消失（Boss转阶段时调用）
    void forceDissipate();

    // 重写 takeDamage 以防止闪烁或击退（如果需要）
    virtual void takeDamage(int damage) override;

    // 死亡逻辑
    virtual void die() override;

    // 设置死亡回调
    void setDeathCallback(const std::function<void()>& callback) { _deathCallback = callback; }
    
    // 设置吸收生命上限的回调（通知Boss）
    void setAbsorbCallback(const std::function<void(int)>& callback) { _absorbCallback = callback; }

    // 无法生成恐卡兹
    virtual bool canSpawnKongKaZiOnDeath() const override { return false; }

protected:
    void loadAnimations();
    void pickNewDirection();
    void checkPlayerCollision();
    void updateFacing();

    // 状态
    bool _isMoving;
    float _idleTimer;      // 初始发呆计时
    float _lifeTimer;      // 30秒生命周期计时
    int _collisionCount;   // 碰撞次数
    int _absorbedCount;    // 吸收总次数
    
    // 碰撞冷却
    float _collisionCooldown;
    
    // 移动方向改变计时
    float _moveChangeTimer;
    
    cocos2d::Rect _roomBounds;
    cocos2d::Vec2 _currentMoveDir;

    // 动画
    cocos2d::Animation* _animIdle;
    cocos2d::Animation* _animMove;
    cocos2d::Animation* _animDie;

    // 回调
    std::function<void()> _deathCallback;
    std::function<void(int)> _absorbCallback;

    // 动作Tag
    static const int BOAT_ACTION_TAG = 0xB0A7;
};

#endif // __BOAT_H__