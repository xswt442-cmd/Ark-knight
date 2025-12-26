#ifndef __KUILONGBOSS_H__
#define __KUILONGBOSS_H__

#include "Enemy.h"
#include "cocos2d.h"
#include "ui/CocosGUI.h"

class KuiLongBoss : public Enemy {
public:
    KuiLongBoss();
    virtual ~KuiLongBoss();

    virtual bool init() override;
    virtual void update(float dt) override;

    CREATE_FUNC(KuiLongBoss);

    // AI 与动画
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

protected:
    void loadAnimations();
    cocos2d::Animation* loadAnimationFrames(const std::string& folder, const std::string& prefix, int maxFrames, float delayPerUnit);

    // ========== 阶段定义 ==========
    enum Phase {
        PHASE_A,            // 初始阶段：隐身、免疫伤害/剧毒，Idle 循环（持续 _phaseADuration）
        TRANSITION_A_TO_B,  // 从 A -> B 的转场（播放转场动画），在转场结束立即进入 PHASE_B
        PHASE_B,            // 可被识别、可受伤、正常移动/攻击
        PHASE_C             // 预留（例如死亡/其它形态）
    };

    Phase _phase;            // 当前阶段（见上面枚举）
    float _phaseTimer;       // 阶段计时器（用于阶段时长计算）
    float _phaseADuration;   // 阶段 A 的持续时长（秒）

    // ========== 动画资源 ==========
    cocos2d::Animation* _animAIdle;         // A 阶段 Idle 循环动画
    cocos2d::Animation* _animAChangeToB;    // A -> B 转场动画
    cocos2d::Animation* _animBMove;         // B 阶段移动循环动画
    cocos2d::Animation* _animBAttack;       // B 阶段攻击动画（单次，用于前摇与命中）
    cocos2d::Animation* _animBChangeToC;    // B -> C（死亡/终结）动画

    // 新增：ChengWuJie 技能动画
    cocos2d::Animation* _animBChengWuJie;   // B 阶段技能 ChengWuJie（23 帧）

    // ========== 动作 Tag（用于 runAction/stopActionByTag） ==========
    static const int KUI_LONG_MOVE_TAG = 0x7F01;    // 移动/循环动画 tag（sprite）
    static const int KUI_LONG_WINDUP_TAG = 0x7F02;  // 攻击前摇动画 tag（sprite）
    static const int KUI_LONG_HIT_TAG = 0x7F03;     // 命中/攻击播放动画 tag（sprite）
    static const int KUI_LONG_CHANGE_TAG = 0x7F04;  // 转场动画 tag（sprite）
    static const int KUI_LONG_DIE_TAG = 0x7F05;     // 死亡动画 tag（sprite）
    static const int KUI_LONG_SKILL_TAG = 0x7F06;   // 技能动画 tag（sprite）
    static const int KUI_LONG_SKILL_DAMAGE_TAG = 0x7F07; // 技能伤害调度 tag（node）

    bool _moveAnimPlaying;   // 当前是否正在播放 move 循环动画（用于避免重复启动）

    // ========== Boss 血条 UI（作为 Boss 子节点，但使用 world-space globalZ） ==========
    cocos2d::ui::LoadingBar* _bossHPBar; // 血条填充条（percent 控制）
    cocos2d::Label* _bossHPLabel;        // 血量文本（例如 "1500/20000"）
    float _bossBarOffsetY;               // 血条垂直偏移（相对于精灵底部）

    // ========== ChengWuJie 技能参数 ==========
    float _skillCooldown;        // 冷却时长（秒）
    float _skillCooldownTimer;   // 冷却计时（秒）
    float _skillRange;           // 触发条件：近战范围（比普通攻击范围大）
    int   _skillDamagePerHit;    // 每次命中伤害
    bool  _skillPlaying;         // 是否正在播放技能（不可被打断）

    // 用于在 sprite 层播放额外技能覆盖图层（与 _sprite 不同，需在析构中清理）
    cocos2d::Sprite* _skillSprite;

    // 启动与判断技能
    bool canUseChengWuJie() const;
    void startChengWuJie(Player* target);
    void resetChengWuJieCooldown();
};

#endif // __KUILONGBOSS_H__