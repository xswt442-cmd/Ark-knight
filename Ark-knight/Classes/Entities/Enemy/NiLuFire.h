#ifndef __NILU_FIRE_H__
#define __NILU_FIRE_H__

#include "Enemy.h"
#include "cocos2d.h"
#include "ui/CocosGUI.h"

class NiLuFire : public Enemy {
public:
    NiLuFire();
    virtual ~NiLuFire();

    virtual bool init() override;
    virtual void update(float dt) override;
    CREATE_FUNC(NiLuFire);

    // 玩家使用治疗术时，恢复 NiLuFire 的血量（由调用方在半径200内调用）
    void onHealedByPlayer(int healAmount);

    // 让 NiLuFire 立即执行一次攻击（用于与 KuiLong 同步）
    // 只有播放 Attack 动画时才在动画回调里真正造成一次伤害
    void performAttackImmediate(int damage);

    // 接收房间边界
    virtual void setRoomBounds(const cocos2d::Rect& bounds) override;

    // NiLuFire 不应被我方普通攻击伤害（由此覆盖）
    virtual void takeDamage(int damage) override;

    // NiLuFire 不计入房间清怪统计（覆盖）
    virtual bool countsForRoomClear() const override { return false; }

    // 禁止被恐卡兹标记/寄生（覆盖）
    virtual bool canSpawnKongKaZiOnDeath() const override { return false; }

    // 查询当前是否处于“正在播放攻击动画/将要造成伤害”的状态
    bool isPerformingAttack() const { return _isPerformingAttack; }

protected:
    void loadAnimations();

    // 自爆（60s 到点触发）——自爆会播放攻击动画并造成一次十字伤害后消失
    void performSelfDestruct();

    // HP UI 显示
    void createHPBar();
    void updateHPBar();

    cocos2d::Animation* _animAttack;
    cocos2d::Animation* _animBurn;

    // HP UI（world-space）
    cocos2d::ui::LoadingBar* _hpBar;
    cocos2d::Label* _hpLabel;

    // 计时
    float _lifetimeTimer;      // 活跃计时（60s）
    float _protectTimer;       // 出场保护期
    bool  _isProtected;

    float _lifeLimit;          // 生命周期（60s）

    // 房间边界
    cocos2d::Rect _roomBounds;

    // 攻击力（可用传入参数或 getAttack()）
    int _attackDamage;

    // 当前是否正在执行一次攻击（仅在播放 Attack 动画并在回调中造成伤害时为 true）
    bool _isPerformingAttack;
};

#endif // __NILU_FIRE_H__