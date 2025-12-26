#include "Chest.h"
#include "Item.h"
#include "Actor/Player.h"
#include "Const/Const.h"
#include "cocos2d.h"

using namespace cocos2d;

Chest* Chest::create(ChestType type, bool randomType)
{
    Chest* chest = new (std::nothrow) Chest();
    if (chest && chest->init(type, randomType))
    {
        chest->autorelease();
        return chest;
    }
    CC_SAFE_DELETE(chest);
    return nullptr;
}

bool Chest::init(ChestType type, bool randomType)
{
    if (!Node::init())
    {
        return false;
    }
    
    _isOpened = false;
    
    // 随机选择宝箱类型
    if (randomType)
    {
        _chestType = (RandomHelper::random_int(0, 1) == 0) ? ChestType::WOODEN : ChestType::IRON;
    }
    else
    {
        _chestType = type;
    }
    
    // 创建宝箱精灵
    std::string chestPath = (_chestType == ChestType::WOODEN) 
        ? "Map/Chest/Wooden_chest.png" 
        : "Map/Chest/Iron_chest.png";
    
    _sprite = Sprite::create(chestPath);
    if (!_sprite)
    {
        GAME_LOG("Failed to create chest sprite: %s", chestPath.c_str());
        return false;
    }
    
    // 缩放到合适大小（2倍地板砖大小）
    float targetSize = Constants::FLOOR_TILE_SIZE * 2.0f;
    float scale = targetSize / _sprite->getContentSize().width;
    _sprite->setScale(scale);
    
    _sprite->setGlobalZOrder(Constants::ZOrder::FLOOR + 2);
    this->addChild(_sprite);
    
    return true;
}

bool Chest::canInteract(Player* player, float interactionDistance) const
{
    if (_isOpened || !player)
    {
        return false;
    }
    
    // 默认交互距离为2格
    if (interactionDistance <= 0.0f)
    {
        interactionDistance = Constants::FLOOR_TILE_SIZE * 2.0f;
    }
    
    float distance = this->getPosition().distance(player->getPosition());
    return distance <= interactionDistance;
}

void Chest::open(Player* player, const std::unordered_map<std::string, int>& ownedItems)
{
    if (_isOpened || !player)
    {
        return;
    }
    
    _isOpened = true;
    
    // 播放打开动画：宝箱慢慢消失（淡出+缩小）
    auto fadeOut = FadeOut::create(0.5f);
    auto scaleDown = ScaleTo::create(0.5f, 0.0f);
    auto spawn = Spawn::create(fadeOut, scaleDown, nullptr);
    auto remove = RemoveSelf::create();
    auto sequence = Sequence::create(spawn, remove, nullptr);
    
    _sprite->runAction(sequence);
    
    // 抽取道具
    const ItemDef* item = ItemLibrary::pickRandom(ownedItems);
    
    if (!item)
    {
        GAME_LOG("Chest opened, but no item available!");
        return;
    }
    
    GAME_LOG("Chest opened! Got item: %s", item->name.c_str());
    
    // 应用道具效果
    applyItemEffect(player, item);
    
    // 显示道具UI
    showItemUI(item, this->getPosition());
}

void Chest::applyItemEffect(Player* player, const ItemDef* item)
{
    if (!player || !item)
    {
        return;
    }
    
    if (item->id == "Knife") {
        // 锈蚀刀片：攻击+15%
        player->multiplyAttack(1.15f);
    }
    else if (item->id == "FirstAidKit") {
        // 急救药箱：最大生命+20%，然后回复20%生命
        player->multiplyMaxHP(1.2f, 0.2f);
    }
    else if (item->id == "Shield") {
        // 坚守盾牌：减伤15%
        player->addDamageReduction(0.15f);
    }
    else if (item->id == "CoinToy") {
        // 投币玩具：攻击间隔-15%
        player->multiplyAttackCooldown(0.85f);
    }
    else if (item->id == "Roses") {
        // 活玫瑰：治疗术+50%
        player->addHealPowerMultiplier(0.5f);
    }
    else if (item->id == "HappyDrink") {
        // 快乐水：MP回复+1/秒
        player->addMPRegenBonus(1.0f);
    }
    else if (item->id == "Revenger") {
        // 复仇者：攻击+30%
        player->multiplyAttack(1.3f);
    }
    else if (item->id == "UnknownInstrument") {
        // 未知仪器：最大生命+40%，然后回复50%生命
        player->multiplyMaxHP(1.4f, 0.5f);
    }
    else if (item->id == "AncientArmour") {
        // 古老的铠甲：减伤30%
        player->addDamageReduction(0.3f);
    }
    else if (item->id == "DaydreamPerfume") {
        // 迷梦香精：MP回复+3/秒
        player->addMPRegenBonus(3.0f);
    }
    else if (item->id == "GoldWine") {
        // 金酒之杯：攻击间隔-30%
        player->multiplyAttackCooldown(0.7f);
    }
    else if (item->id == "KingsSpear") {
        // 国王的新枪：攻击间隔-50%（套装效果暂不实现）
        player->multiplyAttackCooldown(0.5f);
    }
    else if (item->id == "KingsCrown") {
        // 诸王的冠冕：攻击+50%（套装效果暂不实现）
        player->multiplyAttack(1.5f);
    }
    else if (item->id == "KingsHelmet") {
        // 国王的铠甲：最大生命+50%（套装效果暂不实现）
        player->multiplyMaxHP(1.5f, 0.0f);
    }
    else if (item->id == "KingsExtension") {
        // 国王的延伸：MP回复+5/秒，HP回复2%/秒（套装效果暂不实现）
        player->addMPRegenBonus(5.0f);
        player->addHPRegenPercent(0.02f);
    }
}

void Chest::showItemUI(const ItemDef* item, const cocos2d::Vec2& position)
{
    if (!item)
    {
        return;
    }
    
    // 创建道具图标
    auto itemIcon = Sprite::create(item->iconPath);
    if (!itemIcon)
    {
        GAME_LOG("Failed to create item icon: %s", item->iconPath.c_str());
        return;
    }
    
    itemIcon->setPosition(position + Vec2(0, 100));
    itemIcon->setScale(2.0f);
    itemIcon->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL);
    this->getParent()->addChild(itemIcon, Constants::ZOrder::UI_GLOBAL);
    
    // 创建道具名称标签
    auto itemLabel = Label::createWithTTF(item->name, "fonts/msyh.ttf", 24);
    itemLabel->setPosition(itemIcon->getPosition() + Vec2(0, 60));
    itemLabel->setTextColor(Color4B::YELLOW);
    itemLabel->enableOutline(Color4B::BLACK, 2);
    itemLabel->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL);
    this->getParent()->addChild(itemLabel, Constants::ZOrder::UI_GLOBAL);
    
    // 2秒后淡出消失
    auto delay = DelayTime::create(2.0f);
    auto fadeOut = FadeOut::create(1.0f);
    auto remove = RemoveSelf::create();
    auto seq = Sequence::create(delay, fadeOut, remove, nullptr);
    
    itemIcon->runAction(seq->clone());
    itemLabel->runAction(seq->clone());
}
