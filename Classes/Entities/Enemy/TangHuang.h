#ifndef __TANGHUANG_H__
#define __TANGHUANG_H__

#include "Enemy.h"
#include "cocos2d.h"

// 前向声明避免头循环
class Player;

/**
 * 堂皇 TangHuang
 * - 近战 MELEE
 * - 简单追击/巡逻 AI，近战攻击
 */
class TangHuang : public Enemy {
public:
    TangHuang();
    virtual ~TangHuang();

    virtual bool init() override;
    virtual void update(float dt) override;

    CREATE_FUNC(TangHuang);

    // AI 循环
    virtual void executeAI(Player* player, float dt) override;

    // 初始化属性与动画加载
    void setupAttributes();
    void loadAnimations();

    // 攻击/播放攻击动画/死亡
    virtual void attack() override;
    virtual void playAttackAnimation() override;
    virtual void die() override;

    // 设置房间边界
    virtual void setRoomBounds(const cocos2d::Rect& bounds) override { _roomBounds = bounds; _hasRoomBounds = true; }

    // 移动（使用基类 move，但同步动画）
    virtual void move(const cocos2d::Vec2& direction, float dt) override;

    // 对玩家造成伤害（覆写以实现具体伤害逻辑）
    virtual void attackPlayer(Player* player) override;

    // 技能相关接口
    void setSkillCooldown(float seconds) { _skillCooldown = seconds; }
    float getSkillCooldown() const { return _skillCooldown; }
    bool canUseSkill() const { return _skillCooldownTimer <= 0.0f; }
    void useSmokeSkill(); // 触发烟雾技能（内部会处理冷却与延迟）
    void spawnSmoke();    // 实际生成烟雾（会创建视觉并管理 stealth 源）

protected:
    cocos2d::Animation* _moveAnimation;
    cocos2d::Animation* _attackAnimation;
    cocos2d::Animation* _dieAnimation;
    // 技能动画
    cocos2d::Animation* _skillAnimation;

    cocos2d::Rect _roomBounds;
    bool _hasRoomBounds;

    // 攻击时的目标引用（非 owning）
    Player* _attackTarget;

    // 堂皇技能状态
    // 技能冷却（秒），默认为 15s（在构造或 setupAttributes 中设置）
    float _skillCooldown;
    // 当前技能 CD 计时器（>0 表示正在冷却）
    float _skillCooldownTimer;

    // 将创建烟雾的实际逻辑封装为类方法 spawnsmoke()
};

#endif // __TANGHUANG_H__