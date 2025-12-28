#ifndef __BARRIERS_H__
#define __BARRIERS_H__

#include "cocos2d.h"
#include "Core/Constants.h"
#include "Core/GameMacros.h"

class Player;

// 障碍物基类
class Barrier : public cocos2d::Sprite {
public:
    virtual ~Barrier() = default;
    
    // 是否阻挡移动
    virtual bool blocksMovement() const { return false; }
    
    // 是否阻挡子弹
    virtual bool blocksProjectiles() const { return false; }
    
    // 更新障碍物状态
    virtual void updateState(float dt, Player* player) {}
};

// 地刺陷阱：踩上去每秒造成固定伤害，并在触发时变白
class Spike : public Barrier {
public:
    static Spike* create(const std::string& texturePath = "Map/Barrier/Spikes_down.png");
    
    bool blocksMovement() const override { return false; }
    bool blocksProjectiles() const override { return false; }
    
    void updateState(float dt, Player* player) override;
    
private:
    bool initWithTexturePath(const std::string& texturePath);
    void setTriggered(bool triggered);
    
    bool _triggered = false;
    float _damageTimer = 0.0f;
    const float _damageInterval = 1.0f;   // 每秒一次
    const int _damagePerTick = 10;         // 每次10点伤害
};

// 木箱障碍物：阻挡移动和子弹
class Box : public Barrier {
public:
    enum class BoxType {
        NORMAL,  // Box_normal.png
        LIGHT,   // Box_light.png
        DARK     // Box_dark.png
    };
    
    static Box* create(BoxType type = BoxType::NORMAL);
    
    bool blocksMovement() const override { return true; }
    bool blocksProjectiles() const override { return true; }
    
private:
    bool initWithType(BoxType type);
    
    BoxType _boxType;
};

// 石柱障碍物：阻挡移动和子弹
class Pillar : public Barrier {
public:
    enum class PillarType {
        CLEAR,   // Pillar_clear.png
        BROKEN,  // Pillar_broken.png
        GLASSES  // Pillar_glasses.png
    };
    
    static Pillar* create(PillarType type = PillarType::CLEAR);
    
    bool blocksMovement() const override { return true; }
    bool blocksProjectiles() const override { return true; }
    
private:
    bool initWithType(PillarType type);
    
    PillarType _pillarType;
};

#endif // __BARRIERS_H__
