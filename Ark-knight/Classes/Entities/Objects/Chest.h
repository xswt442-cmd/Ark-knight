#ifndef __CHEST_H__
#define __CHEST_H__

#include "cocos2d.h"
#include <string>
#include <unordered_map>

class Player;
struct ItemDef;

// 宝箱类：管理宝箱的创建、交互和道具奖励

class Chest : public cocos2d::Node
{
public:
    enum class ChestType {
        WOODEN,  // 木质宝箱
        IRON     // 铁质宝箱
    };
    
    // 创建宝箱
    static Chest* create(ChestType type = ChestType::WOODEN, bool randomType = true);
    
    virtual bool init(ChestType type, bool randomType);
    
    // 检测玩家是否可以与宝箱交互
    bool canInteract(Player* player, float interactionDistance = 0.0f) const;
    
    // 打开宝箱：播放动画、抽取道具、生成掉落物
    cocos2d::Vector<class ItemDrop*> open(const std::unordered_map<std::string, int>& ownedItems);
    
    // Getter
    bool isOpened() const { return _isOpened; }
    cocos2d::Sprite* getSprite() const { return _sprite; }
    
private:
    cocos2d::Sprite* _sprite;  // 宝箱精灵
    bool _isOpened;            // 是否已打开
    ChestType _chestType;      // 宝箱类型
};

#endif // __CHEST_H__
