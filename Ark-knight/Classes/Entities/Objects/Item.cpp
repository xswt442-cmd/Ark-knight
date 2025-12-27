#include "Item.h"
#include "cocos2d.h"
#include "../Player/Player.h"

using namespace cocos2d;

static std::vector<ItemDef> s_items;

static void ensureItems() {
    if (!s_items.empty()) return;

    // 低阶
    s_items.push_back({"Knife", u8"锈蚀刀片", ItemRarity::LOW, "Property/LowLevel/Knife.png", 3, u8"角色攻击+15%"});
    s_items.push_back({"FirstAidKit", u8"急救药箱", ItemRarity::LOW, "Property/LowLevel/First_Aid_Kit.png", 3, u8"最大生命值+20%，然后回复20%生命值"});
    s_items.push_back({"Shield", u8"坚守盾牌", ItemRarity::LOW, "Property/LowLevel/Shield.png", 3, u8"获得15%减伤"});
    s_items.push_back({"CoinToy", u8"投币玩具", ItemRarity::LOW, "Property/LowLevel/CoinToy.png", 3, u8"攻击间隔-15%"});
    s_items.push_back({"Roses", u8"活玫瑰", ItemRarity::LOW, "Property/LowLevel/Roses.png", 3, u8"治疗术治疗量+50%"});
    s_items.push_back({"HappyDrink", u8"快乐水", ItemRarity::LOW, "Property/LowLevel/Happy_Juice.png", 3, u8"法力值恢复速度+1/秒"});

    // 高阶
    s_items.push_back({"Revenger", u8"复仇者", ItemRarity::HIGH, "Property/HighLevel/Revenger.png", 2, u8"角色攻击+30%"});
    s_items.push_back({"UnknownInstrument", u8"未知仪器", ItemRarity::HIGH, "Property/HighLevel/Unknown_instrument.png", 2, u8"最大生命值+40%，然后恢复50%生命值"});
    s_items.push_back({"AncientArmour", u8"古老的铠甲", ItemRarity::HIGH, "Property/HighLevel/Ancient_armour.png", 2, u8"减伤30%"});
    s_items.push_back({"DaydreamPerfume", u8"迷梦香精", ItemRarity::HIGH, "Property/HighLevel/Daydream_Perfume.png", 2, u8"法力值恢复速度+3/秒"});
    s_items.push_back({"GoldWine", u8"金酒之杯", ItemRarity::HIGH, "Property/HighLevel/Gold_Wine.png", 2, u8"攻击间隔-30%"});

    // 国王藏品（每件1次）
    s_items.push_back({"KingsSpear", u8"国王的新枪", ItemRarity::KING, "Property/HighLevel/Kings_Spear.png", 1, u8"攻击间隔-50%，集齐3件国王套后额外获得攻击+100%"});
    s_items.push_back({"KingsCrown", u8"诸王的冠冕", ItemRarity::KING, "Property/HighLevel/Kings_Crown.png", 1, u8"攻击+50%，集齐3件国王套后改为+150%"});
    s_items.push_back({"KingsHelmet", u8"国王的铠甲", ItemRarity::KING, "Property/HighLevel/Kings_helmet.png", 1, u8"最大生命+50%，集齐3件国王套后额外获得50%减伤"});
    s_items.push_back({"KingsExtension", u8"国王的延伸", ItemRarity::KING, "Property/HighLevel/Kings_extension.png", 1, u8"法力值恢复速度+5/秒，每秒恢复最大生命2%生命值，集齐3件国王套后额外获得50%减伤"});
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

void ItemLibrary::applyItemEffect(const std::string& itemId, Player* player)
{
    if (!player)
    {
        return;
    }
    
    if (itemId == "Knife") {
        // 锈蚀刀片：攻击+15%
        player->multiplyAttack(1.15f);
    }
    else if (itemId == "FirstAidKit") {
        // 急救药箱：最大生命+20%，然后回复20%生命
        player->multiplyMaxHP(1.2f, 0.2f);
    }
    else if (itemId == "Shield") {
        // 坚守盾牌：减伤15%
        player->addDamageReduction(0.15f);
    }
    else if (itemId == "CoinToy") {
        // 投币玩具：攻击间隔-15%
        player->multiplyAttackCooldown(0.85f);
    }
    else if (itemId == "Roses") {
        // 活玫瑰：治疗术+50%
        player->addHealPowerMultiplier(0.5f);
    }
    else if (itemId == "HappyDrink") {
        // 快乐水：MP回复+1/秒
        player->addMPRegenBonus(1.0f);
    }
    else if (itemId == "Revenger") {
        // 复仇者：攻击+30%
        player->multiplyAttack(1.3f);
    }
    else if (itemId == "UnknownInstrument") {
        // 未知仪器：最大生命+40%，然后回复50%生命
        player->multiplyMaxHP(1.4f, 0.5f);
    }
    else if (itemId == "AncientArmour") {
        // 古老的铠甲：减伤30%
        player->addDamageReduction(0.3f);
    }
    else if (itemId == "DaydreamPerfume") {
        // 迷梦香精：MP回复+3/秒
        player->addMPRegenBonus(3.0f);
    }
    else if (itemId == "GoldWine") {
        // 金酒之杯：攻击间隔-30%
        player->multiplyAttackCooldown(0.7f);
    }
    else if (itemId == "KingsSpear") {
        // 国王的新枪：攻击间隔-50%
        player->multiplyAttackCooldown(0.5f);
    }
    else if (itemId == "KingsCrown") {
        // 诸王的冠冕：攻击+50%
        player->multiplyAttack(1.5f);
    }
    else if (itemId == "KingsHelmet") {
        // 国王的铠甲：最大生命+50%
        player->multiplyMaxHP(1.5f, 0.0f);
    }
    else if (itemId == "KingsExtension") {
        // 国王的延伸：MP回复+5/秒，HP回复2%/秒
        player->addMPRegenBonus(5.0f);
        player->addHPRegenPercent(0.02f);
    }
    
    CCLOG("ItemLibrary: Applied item effect for %s", itemId.c_str());
}