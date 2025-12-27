#ifndef __KUILONGBOSS_H__
#define __KUILONGBOSS_H__

#include "Enemy.h"
#include "cocos2d.h"
#include "ui/CocosGUI.h"

class NiLuFire; // 前向声明
class Boat;     // 前向声明
class Room;     // 前向声明

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

    // 重写移动以控制移动动画
    virtual void move(const cocos2d::Vec2& direction, float dt) override;

    // 伤害处理
    virtual void takeDamage(int damage) override;
    virtual int takeDamageReported(int damage) override;
    virtual void die() override;

    virtual bool canSpawnKongKaZiOnDeath() const override { return false; }
    virtual bool isPoisonable() const override;
    virtual void setRoomBounds(const cocos2d::Rect& bounds) override;
    
    // 设置三阶段房间（由GameScene在生成Boss时传入）
    void setPhase3Room(Room* room) { _phase3Room = room; }

    // 供 Boat 死亡时调用
    void endChengSanShen();
    
    // 供 Boat 吸收玩家血量时调用
    void reportBoatAbsorb(int amount);

protected:
    void loadAnimations();
    cocos2d::Animation* loadAnimationFrames(const std::string& folder, const std::string& prefix, int maxFrames, float delayPerUnit);

    // ========== 阶段定义 ==========
    enum Phase {
        PHASE_A,
        TRANSITION_A_TO_B,
        PHASE_B,
        TRANSITION_B_TO_C, // 新增：二转三过渡阶段
        PHASE_C,
        SKILL_CHENG_SAN_SHEN
    };

    Phase _phase;
    float _phaseTimer;
    float _phaseADuration;

    // ========== 召唤与机制 ==========
    bool _threshold75Triggered;
    bool _threshold50Triggered;
    bool _threshold25Triggered;
    
    float _chengSanShenTimer;
    float _chengSanShenDuration;
    bool _chengSanShenEnding; 
    Boat* _summonedBoat;

    // 2.3: 二阶段召唤相关
    float _phaseBSummonTimer;
    float _phaseBSummonInterval;
    int _escalationLevel; // 记录承三身触发次数
    int _phaseBSummonCount; // 记录二阶段定期召唤次数
    int _totalBoatAbsorbed; // 记录所有 Boat 吸收的玩家生命上限

    void checkChengSanShenTrigger();
    void startChengSanShen();
    void updateChengSanShen(float dt);
    
    // 新增辅助函数
    void spawnPhaseBMinions(bool isPeriodic);
    void spawnChengSanShenMinions();
    void spawnEnemyHelper(const std::string& type, int count, float radius);
    void startTransitionBToC(); // 开始二转三流程
    void forceKillAllAndTransition(); // 2.4: 强制击杀并转阶段

    // ========== 动画资源 ==========
    cocos2d::Animation* _animAIdle;
    cocos2d::Animation* _animAChangeToB;
    cocos2d::Animation* _animBMove;
    cocos2d::Animation* _animBAttack;
    cocos2d::Animation* _animBChangeToC;
    cocos2d::Animation* _animBChengWuJie;
    
    cocos2d::Animation* _animCSS_Start;
    cocos2d::Animation* _animCSS_Idle;
    cocos2d::Animation* _animCSS_End;
    
    cocos2d::Animation* _animCDie; // 三阶段死亡动画

    // ========== 动作 Tag ==========
    static const int KUI_LONG_MOVE_TAG = 0x7F01;
    static const int KUI_LONG_WINDUP_TAG = 0x7F02;
    static const int KUI_LONG_HIT_TAG = 0x7F03;
    static const int KUI_LONG_CHANGE_TAG = 0x7F04;
    static const int KUI_LONG_DIE_TAG = 0x7F05;
    static const int KUI_LONG_SKILL_TAG = 0x7F06;
    static const int KUI_LONG_SKILL_DAMAGE_TAG = 0x7F07;
    static const int KUI_LONG_CSS_TAG = 0x7F08;

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
    bool  _skillDamageScheduled;

    bool canUseChengWuJie() const;
    void startChengWuJie(Player* target);
    void resetChengWuJieCooldown();

    // ========== NiLuFire 相关 ==========
    cocos2d::Rect _roomBounds;
    Room* _phase3Room; // 三阶段目标房间
    float _niluSpawnTimer;
    float _niluSpawnInterval;
    int _niluSpawnPerInterval;
    int _niluMaxPerRoom;
    float _niluMinDistance;

    int countNiLuFiresInRoom() const;
    bool isPositionValidForNiLu(const cocos2d::Vec2& pos) const;
    void trySpawnNiLuFires();
};

#endif // __KUILONGBOSS_H__