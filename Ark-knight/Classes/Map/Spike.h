#ifndef __SPIKE_H__
#define __SPIKE_H__

#include "cocos2d.h"
#include "Core/Constants.h"
#include "Core/GameMacros.h"

class Player;

/**
 * 地刺陷阱：踩上去每秒造成固定伤害，并在触发时变白
 */
class Spike : public cocos2d::Sprite {
public:
    static Spike* createSpike(const std::string& texturePath = "Map/Barrier/Spikes_down.png");
    
    /**
     * 更新地刺状态
     * @param dt 帧间隔
     * @param isStepped 玩家是否踩在上面
     * @param player 玩家指针，用于施加伤害
     */
    void updateState(float dt, bool isStepped, Player* player);
    
private:
    bool initWithTexturePath(const std::string& texturePath);
    void setTriggered(bool triggered);
    
    bool _triggered = false;
    float _damageTimer = 0.0f;
    const float _damageInterval = 1.0f;   // 每秒一次
    const int _damagePerTick = 10;         // 每次10点伤害
};

#endif // __SPIKE_H__
