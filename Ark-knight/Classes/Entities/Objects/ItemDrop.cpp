#include "ItemDrop.h"
#include "Entities/Player/Player.h"
#include "Core/Constants.h"

using namespace cocos2d;

ItemDrop* ItemDrop::create(const ItemDef* itemDef)
{
    ItemDrop* drop = new (std::nothrow) ItemDrop();
    if (drop && drop->init(itemDef))
    {
        drop->autorelease();
        return drop;
    }
    CC_SAFE_DELETE(drop);
    return nullptr;
}

bool ItemDrop::init(const ItemDef* itemDef)
{
    if (!Node::init() || !itemDef)
    {
        return false;
    }
    
    _itemDef = itemDef;
    _isPickedUp = false;
    
    // 创建道具图标
    _sprite = Sprite::create(_itemDef->iconPath);
    if (!_sprite)
    {
        GAME_LOG("Failed to create item drop sprite: %s", _itemDef->iconPath.c_str());
        return false;
    }
    
    // 缩放到合适大小（1.5倍地板砖大小）
    float targetSize = Constants::FLOOR_TILE_SIZE * 1.5f;
    float scale = targetSize / _sprite->getContentSize().width;
    _sprite->setScale(scale);
    
    _sprite->setGlobalZOrder(Constants::ZOrder::FLOOR + 2);
    this->addChild(_sprite);
    
    // 添加浮动动画效果
    auto moveUp = MoveBy::create(1.0f, Vec2(0, 10));
    auto moveDown = moveUp->reverse();
    auto sequence = Sequence::create(moveUp, moveDown, nullptr);
    auto repeat = RepeatForever::create(sequence);
    _sprite->runAction(repeat);
    
    return true;
}

bool ItemDrop::canPickup(Player* player, float interactionDistance) const
{
    if (_isPickedUp || !player)
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

void ItemDrop::pickup(Player* player)
{
    if (_isPickedUp || !player)
    {
        return;
    }
    
    _isPickedUp = true;
    
    GAME_LOG("Picked up item: %s", _itemDef->name.c_str());
    
    // 应用道具效果
    applyEffect(player);
    
    // 播放拾取动画并移除
    auto fadeOut = FadeOut::create(0.3f);
    auto scaleUp = ScaleTo::create(0.3f, _sprite->getScale() * 1.5f);
    auto spawn = Spawn::create(fadeOut, scaleUp, nullptr);
    auto remove = RemoveSelf::create();
    auto sequence = Sequence::create(spawn, remove, nullptr);
    
    this->runAction(sequence);
}

void ItemDrop::applyEffect(Player* player)
{
    if (!player || !_itemDef)
    {
        return;
    }
    
    if (_itemDef->id == "Knife") {
        // 锈蚀刀片：攻击+15%
        player->multiplyAttack(1.15f);
    }
    else if (_itemDef->id == "FirstAidKit") {
        // 急救药箱：最大生命+20%，然后回复20%生命
        player->multiplyMaxHP(1.2f, 0.2f);
    }
    else if (_itemDef->id == "Shield") {
        // 坚守盾牌：减伤15%
        player->addDamageReduction(0.15f);
    }
    else if (_itemDef->id == "CoinToy") {
        // 投币玩具：攻击间隔-15%
        player->multiplyAttackCooldown(0.85f);
    }
    else if (_itemDef->id == "Roses") {
        // 活玫瑰：治疗术+50%
        player->addHealPowerMultiplier(0.5f);
    }
    else if (_itemDef->id == "HappyDrink") {
        // 快乐水：MP回复+1/秒
        player->addMPRegenBonus(1.0f);
    }
    else if (_itemDef->id == "Revenger") {
        // 复仇者：攻击+30%
        player->multiplyAttack(1.3f);
    }
    else if (_itemDef->id == "UnknownInstrument") {
        // 未知仪器：最大生命+40%，然后回复50%生命
        player->multiplyMaxHP(1.4f, 0.5f);
    }
    else if (_itemDef->id == "AncientArmour") {
        // 古老的铠甲：减伤30%
        player->addDamageReduction(0.3f);
    }
    else if (_itemDef->id == "DaydreamPerfume") {
        // 迷梦香精：MP回复+3/秒
        player->addMPRegenBonus(3.0f);
    }
    else if (_itemDef->id == "GoldWine") {
        // 金酒之杯：攻击间隔-30%
        player->multiplyAttackCooldown(0.7f);
    }
    else if (_itemDef->id == "KingsSpear") {
        // 国王的新枪：攻击间隔-50%（套装效果暂不实现）
        player->multiplyAttackCooldown(0.5f);
    }
    else if (_itemDef->id == "KingsCrown") {
        // 诸王的冠冕：攻击+50%（套装效果暂不实现）
        player->multiplyAttack(1.5f);
    }
    else if (_itemDef->id == "KingsHelmet") {
        // 国王的铠甲：最大生命+50%（套装效果暂不实现）
        player->multiplyMaxHP(1.5f, 0.0f);
    }
    else if (_itemDef->id == "KingsExtension") {
        // 国王的延伸：MP回复+5/秒，HP回复2%/秒（套装效果暂不实现）
        player->addMPRegenBonus(5.0f);
        player->addHPRegenPercent(0.02f);
    }
}
