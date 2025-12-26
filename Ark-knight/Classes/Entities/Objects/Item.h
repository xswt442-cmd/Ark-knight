#ifndef __ITEM_H__
#define __ITEM_H__

#include <string>
#include <vector>
#include <unordered_map>

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
};

/**
 * 道具静态库：提供道具定义和随机抽取能力
 */
class ItemLibrary {
public:
    static const std::vector<ItemDef>& all();

    /**
     * 按稀有度概率抽取道具（低阶60%，高阶30%，国王10%），同时考虑堆叠上限
     * @return 返回道具指针，如果没有可用道具则返回nullptr
     */
    static const ItemDef* pickRandom(const std::unordered_map<std::string, int>& ownedCounts);
};

#endif // __ITEM_H__