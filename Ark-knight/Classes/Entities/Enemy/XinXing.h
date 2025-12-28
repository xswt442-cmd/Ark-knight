#ifndef __XINXING_H__
#define __XINXING_H__

#include "Enemy.h"
#include "cocos2d.h"

/**
 * 新硎（XinXing）
 * - 地面近战精英（MELEE）
 * - 发现玩家后追击并近身攻击（高攻速、高伤害）
 * - 死亡后生成 3 个 IronLance
 * - 精灵与碰撞体放大 2 倍
 * - 遵守房间边界
 */
class XinXing : public Enemy {
public:
    XinXing();
    virtual ~XinXing();

    virtual bool init() override;
    virtual void update(float dt) override;

    CREATE_FUNC(XinXing);

    // AI 行为（覆盖基类）
    virtual void executeAI(Player* player, float dt) override;

    // 动画与属性
    void setupAttributes();
    void loadAnimations();

    // 覆写行为
    virtual void attack() override;
    virtual void playAttackAnimation() override;
    virtual void die() override;

    // 房间边界
    virtual void setRoomBounds(const cocos2d::Rect& bounds) override { _roomBounds = bounds; _hasRoomBounds = true; }

    // 控制移动动画与朝向
    virtual void move(const cocos2d::Vec2& direction, float dt) override;

protected:
    cocos2d::Animation* _moveAnimation;
    cocos2d::Animation* _attackAnimation;
    cocos2d::Animation* _dieAnimation;

    cocos2d::Rect _roomBounds;
    bool _hasRoomBounds;
};

#endif // __XINXING_H__