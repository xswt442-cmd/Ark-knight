#ifndef __CHEST_H__
#define __CHEST_H__

#include "cocos2d.h"
#include <string>
#include <unordered_map>

class Player;
struct ItemDef;

/**
 * 宝箱类：管理宝箱的创建、交互和道具奖励
 */
class Chest : public cocos2d::Node
{
public:
    enum class ChestType {
        WOODEN,  // 木质宝箱
        IRON     // 铁质宝箱
    };
    
    /**
     * 创建宝箱
     * @param type 宝箱类型（随机时传入WOODEN即可）
     * @param randomType 是否随机选择宝箱类型
     */
    static Chest* create(ChestType type = ChestType::WOODEN, bool randomType = true);
    
    virtual bool init(ChestType type, bool randomType);
    
    /**
     * 检测玩家是否可以与宝箱交互
     * @param player 玩家指针
     * @param interactionDistance 交互距离（默认2倍地板砖大小）
     */
    bool canInteract(Player* player, float interactionDistance = 0.0f) const;
    
    /**
     * 打开宝箱：播放动画、抽取道具、应用效果、显示UI
     * @param player 玩家指针，用于应用道具效果
     * @param ownedItems 玩家已拥有的道具计数（用于堆叠限制）
     */
    void open(Player* player, const std::unordered_map<std::string, int>& ownedItems);
    
    /**
     * 应用道具效果到玩家
     */
    void applyItemEffect(Player* player, const ItemDef* item);
    
    /**
     * 显示获得道具的UI
     */
    void showItemUI(const ItemDef* item, const cocos2d::Vec2& position);
    
    // Getter
    bool isOpened() const { return _isOpened; }
    cocos2d::Sprite* getSprite() const { return _sprite; }
    
private:
    cocos2d::Sprite* _sprite;  // 宝箱精灵
    bool _isOpened;            // 是否已打开
    ChestType _chestType;      // 宝箱类型
};

#endif // __CHEST_H__
