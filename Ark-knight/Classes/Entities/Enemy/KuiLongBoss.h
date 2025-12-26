#ifndef __KUILONGBOSS_H__
#define __KUILONGBOSS_H__

#include "Enemy.h"
#include "cocos2d.h"
#include "ui/CocosGUI.h"

class NiLuFire; // 前向声明

class KuiLongBoss : public Enemy {
public:
    KuiLongBoss();
    virtual ~KuiLongBoss();

    virtual bool init() override;
    virtual void update(float dt) override;

    CREATE_FUNC(KuiLongBoss);

    // AI 行为
    virtual void executeAI(Player* player, float dt) override;
    virtual void attack() override;
    virtual void playAttackAnimation() override;

    // 覆写移动以控制移动动画（防止攻击期间移动/恢复移动动画）
    virtual void move(const cocos2d::Vec2& direction, float dt) override;

    // 伤害与死亡
    virtual void takeDamage(int damage) override;
    virtual int takeDamageReported(int damage) override;
    virtual void die() override;

    // Boss 不会在死亡时生成 KongKaZi（不能被寄生）
    virtual bool canSpawnKongKaZiOnDeath() const override { return false; }

    // Boss 在阶段 A / 转场阶段免疫剧毒
    virtual bool isPoisonable() const override;

    // 接收房间边界（用于 NiLu 生成位置限制）
    virtual void setRoomBounds(const cocos2d::Rect& bounds) override;

protected:
    void loadAnimations();
    cocos2d::Animation* loadAnimationFrames(const std::string& folder, const std::string& prefix, int maxFrames, float delayPerUnit);

    // ========== 阶段定义 ==========
    enum Phase {
        PHASE_A,
        TRANSITION_A_TO_B,
        PHASE_B,
        PHASE_C
    };

    Phase _phase;
    float _phaseTimer;
    float _phaseADuration;

    // ========== 动画资源 ==========
    cocos2d::Animation* _animAIdle;
    cocos2d::Animation* _animAChangeToB;
    cocos2d::Animation* _animBMove;
    cocos2d::Animation* _animBAttack;
    cocos2d::Animation* _animBChangeToC;
    cocos2d::Animation* _animBChengWuJie;

    // ========== 动作 Tag ==========
    static const int KUI_LONG_MOVE_TAG = 0x7F01;
    static const int KUI_LONG_WINDUP_TAG = 0x7F02;
    static const int KUI_LONG_HIT_TAG = 0x7F03;
    static const int KUI_LONG_CHANGE_TAG = 0x7F04;
    static const int KUI_LONG_DIE_TAG = 0x7F05;
    static const int KUI_LONG_SKILL_TAG = 0x7F06;
    static const int KUI_LONG_SKILL_DAMAGE_TAG = 0x7F07;

    bool _moveAnimPlaying;

    // Boss 血条 UI
    cocos2d::ui::LoadingBar* _bossHPBar;
    cocos2d::Label* _bossHPLabel;
    float _bossBarOffsetY;

    // ChengWuJie 技能参数
    float _skillCooldown;
    float _skillCooldownTimer;
    float _skillRange;
    int   _skillDamagePerHit;
    bool  _skillPlaying;
    cocos2d::Sprite* _skillSprite;

    bool canUseChengWuJie() const;
    void startChengWuJie(Player* target);
    void resetChengWuJieCooldown();

    // ========== NiLuFire 管理 ==========
    cocos2d::Rect _roomBounds; // 接收房间 walkable area
    float _niluSpawnTimer;
    float _niluSpawnInterval; // 10s
    int _niluSpawnPerInterval; // 2
    int _niluMaxPerRoom; // 12
    float _niluMinDistance; // 50.0f

    // 辅助：count & spawn
    int countNiLuFiresInRoom() const;
    bool isPositionValidForNiLu(const cocos2d::Vec2& pos) const;
    void trySpawnNiLuFires(); // 在 update 中调用
};

#endif // __KUILONGBOSS_H__