#include "Item.h"
#include "cocos2d.h"

using namespace cocos2d;

static std::vector<ItemDef> s_items;

static void ensureItems() {
    if (!s_items.empty()) return;

    // 低阶
    s_items.push_back({"Knife", "锈蚀刀片", ItemRarity::LOW, "Property/LowLevel/Knife.png", 3});
    s_items.push_back({"FirstAidKit", "急救药箱", ItemRarity::LOW, "Property/LowLevel/First_Aid_Kit.png", 3});
    s_items.push_back({"Shield", "坚守盾牌", ItemRarity::LOW, "Property/LowLevel/Shield.png", 3});
    s_items.push_back({"CoinToy", "投币玩具", ItemRarity::LOW, "Property/LowLevel/CoinToy.png", 3});
    s_items.push_back({"Roses", "活玫瑰", ItemRarity::LOW, "Property/LowLevel/Roses.png", 3});
    s_items.push_back({"HappyDrink", "快乐水", ItemRarity::LOW, "Property/LowLevel/Happy_Juice.png", 3});

    // 高阶
    s_items.push_back({"Revenger", "复仇者", ItemRarity::HIGH, "Property/HighLevel/Revenger.png", 2});
    s_items.push_back({"UnknownInstrument", "未知仪器", ItemRarity::HIGH, "Property/HighLevel/Unknown_instrument.png", 2});
    s_items.push_back({"AncientArmour", "古老的铠甲", ItemRarity::HIGH, "Property/HighLevel/Ancient_armour.png", 2});
    s_items.push_back({"DaydreamPerfume", "迷梦香精", ItemRarity::HIGH, "Property/HighLevel/Daydream_Perfume.png", 2});
    s_items.push_back({"GoldWine", "金酒之杯", ItemRarity::HIGH, "Property/HighLevel/Gold_Wine.png", 2});

    // 国王藏品（每件1次）
    s_items.push_back({"KingsSpear", "国王的新枪", ItemRarity::KING, "Property/HighLevel/Kings_Spear.png", 1});
    s_items.push_back({"KingsCrown", "诸王的冠冕", ItemRarity::KING, "Property/HighLevel/Kings_Crown.png", 1});
    s_items.push_back({"KingsHelmet", "国王的铠甲", ItemRarity::KING, "Property/HighLevel/Kings_helmet.png", 1});
    s_items.push_back({"KingsExtension", "国王的延伸", ItemRarity::KING, "Property/HighLevel/Kings_extension.png", 1});
}

const std::vector<ItemDef>& ItemLibrary::all() {
    ensureItems();
    return s_items;
}

static std::vector<const ItemDef*> filterByRarity(ItemRarity rarity, const std::unordered_map<std::string, int>& owned) {
    ensureItems();
    std::vector<const ItemDef*> result;
    for (auto& def : s_items) {
        if (def.rarity != rarity) continue;
        auto it = owned.find(def.id);
        int have = (it != owned.end()) ? it->second : 0;
        if (have < def.maxStack) result.push_back(&def);
    }
    return result;
}

const ItemDef* ItemLibrary::pickRandom(const std::unordered_map<std::string, int>& ownedCounts) {
    ensureItems();

    // 先按堆叠限制过滤可用道具
    float r = CCRANDOM_0_1();
    ItemRarity target;
    if (r < 0.6f) target = ItemRarity::LOW;
    else if (r < 0.9f) target = ItemRarity::HIGH;
    else target = ItemRarity::KING;

    auto candidate = filterByRarity(target, ownedCounts);
    if (candidate.empty()) {
        // 如果目标稀有度没有可选，回退到所有可选道具
        std::vector<const ItemDef*> fallback;
        for (auto& def : s_items) {
            auto it = ownedCounts.find(def.id);
            int have = (it != ownedCounts.end()) ? it->second : 0;
            if (have < def.maxStack) fallback.push_back(&def);
        }
        if (fallback.empty()) return nullptr;
        int idx = cocos2d::RandomHelper::random_int(0, static_cast<int>(fallback.size()) - 1);
        return fallback[idx];
    }

    int idx = cocos2d::RandomHelper::random_int(0, static_cast<int>(candidate.size()) - 1);
    return candidate[idx];
}