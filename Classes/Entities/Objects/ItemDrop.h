#ifndef __ITEM_DROP_H__
#define __ITEM_DROP_H__

#include "cocos2d.h"
#include "Item.h"

class Player;

// 掉落道具类：地上可拾取的道具物品
class ItemDrop : public cocos2d::Node
{
public:
    // 创建掉落道具
    static ItemDrop* create(const ItemDef* itemDef);
    
    virtual bool init(const ItemDef* itemDef);
    
    // 检测玩家是否可以拾取 (interactionDistance 交互距离，默认2倍地板砖大小)
    bool canPickup(Player* player, float interactionDistance = 0.0f) const;
    
    // 拾取道具：应用效果并移除自身，返回道具定义用于在UI显示
    const ItemDef* pickup(Player* player);
    
    // 应用道具效果到玩家
    void applyEffect(Player* player);
    
    // Getter
    bool isPickedUp() const { return _isPickedUp; }
    const ItemDef* getItemDef() const { return _itemDef; }
    cocos2d::Sprite* getSprite() const { return _sprite; }
    
private:
    const ItemDef* _itemDef;   // 道具定义
    cocos2d::Sprite* _sprite;  // 道具图标
    bool _isPickedUp;          // 是否已拾取
};

#endif // __ITEM_DROP_H__
