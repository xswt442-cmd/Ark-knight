#ifndef __PORTAL_H__
#define __PORTAL_H__

#include "cocos2d.h"

class Player;

// 传送门类：用于关卡间的传送
class Portal : public cocos2d::Node
{
public:
    static Portal* create();
    virtual bool init() override;
    
    // 检测玩家是否可以交互
    bool canInteract(Player* player, float interactionDistance = 0.0f) const;
    
    // 获取主体精灵
    cocos2d::Sprite* getSprite() const { return _portalSprite; }
    
private:
    cocos2d::Sprite* _portalSprite;       // 传送门主体
    cocos2d::Sprite* _lightingSprite;     // 闪电特效
};

#endif // __PORTAL_H__
