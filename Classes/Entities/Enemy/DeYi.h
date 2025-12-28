#ifndef __DEYI_H__
#define __DEYI_H__

#include "Enemy.h"

// 前向声明，避免在头文件中包含 Player.h，减小编译依赖
class Player;

/**
 * 得意 - 会靠近玩家并自爆的小怪
 * 特点：
 *  - 地面单位
 *  - 发现玩家后靠近，进入攻击范围立即自爆并死亡（爆炸造成大量伤害）
 */
class DeYi : public Enemy {
public:
    DeYi();
    virtual ~DeYi();

    virtual bool init() override;
    CREATE_FUNC(DeYi);

    // 更新（限制房间范围）
    virtual void update(float dt) override;

    // 设置房间边界（覆写以接收并应用房间边界）
    virtual void setRoomBounds(const cocos2d::Rect& bounds) override;

    // 与 Ayao 类似的视觉/行为函数
    void setupDeYiAttributes();
    void loadAnimations();

    // DeYi 不执行常规攻击，而是接近后自爆，所以覆写 AI 流程
    virtual void executeAI(Player* player, float dt) override;

    // 覆写 die：在被击杀时触发爆炸（若尚未爆炸）
    virtual void die() override;

    virtual void move(const cocos2d::Vec2& direction, float dt) override;

protected:
    // 动画资源
    cocos2d::Animation* _moveAnimation;
    cocos2d::Animation* _dieAnimation;

    // 房间边界
    cocos2d::Rect _roomBounds;
    bool _hasRoomBounds;

    // 是否已经触发爆炸（避免重复）
    bool _hasExploded;

    // 内部：执行爆炸效果（视觉 + 对玩家造成伤害）
    void doExplosion();
};

#endif // __DEYI_H__