#ifndef __ITEM_H__
#define __ITEM_H__

#include <string>
#include <vector>
#include <unordered_map>

class Player;  // 前向声明

enum class ItemRarity {
    LOW,
    HIGH,
    KING
};

struct ItemDef {
    std::string id;
    std::string name;
    ItemRarity rarity;
    std::string iconPath;
    int maxStack;
    std::string description;  // 效果描述
};

// 道具静态库：提供道具定义和随机抽取能力
class ItemLibrary {
public:
    static const std::vector<ItemDef>& all();

    // 按稀有度概率抽取道具（低阶60%，高阶30%，国王10%），同时考虑堆叠上限
    static const ItemDef* pickRandom(const std::unordered_map<std::string, int>& ownedCounts);
    
    // 根据道具ID应用效果到玩家（用于场景切换时恢复道具效果）
    static void applyItemEffect(const std::string& itemId, Player* player);
};

#endif // __ITEM_H__