#include "Chest.h"
#include "Entities/Player/Player.h"
#include "Entities/Objects/ItemDrop.h"
#include "Entities/Objects/Item.h"
#include "Core/Constants.h"
#include <random>

USING_NS_CC;

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
    _chestType = randomType ? (rand() % 2 == 0 ? ChestType::WOODEN : ChestType::IRON) : type;
    
    std::string spritePath = (_chestType == ChestType::WOODEN) 
        ? "Map/Chest/Wooden_chest.png" 
        : "Map/Chest/Iron_chest.png";
    
    _sprite = Sprite::create(spritePath);
    if (!_sprite)
    {
        CCLOG("Chest: Failed to load sprite from %s", spritePath.c_str());
    }
    
    if (_sprite)
    {
        _sprite->setAnchorPoint(Vec2(0.5f, 0.0f));
        _sprite->setGlobalZOrder(Constants::ZOrder::ITEMS);
        this->addChild(_sprite);
    }
    
    return true;
}

bool Chest::canInteract(Player* player, float interactionDistance) const
{
    if (_isOpened || !player)
    {
        return false;
    }
    
    float distance = this->getPosition().distance(player->getPosition());
    float checkDistance = (interactionDistance > 0) ? interactionDistance : Constants::FLOOR_TILE_SIZE * 2.0f;
    return distance <= checkDistance;
}

cocos2d::Vector<ItemDrop*> Chest::open(const std::unordered_map<std::string, int>& ownedItems)
{
    cocos2d::Vector<ItemDrop*> drops;
    
    if (_isOpened)
    {
        return drops;
    }
    
    _isOpened = true;
    
    // 播放打开动画
    if (_sprite)
    {
        auto fadeOut = FadeOut::create(0.5f);
        auto scaleDown = ScaleTo::create(0.5f, 0.0f);
        auto spawn = Spawn::create(fadeOut, scaleDown, nullptr);
        auto remove = RemoveSelf::create();
        auto sequence = Sequence::create(spawn, remove, nullptr);
        _sprite->runAction(sequence);
    }
    
    // 生成掉落物品：从 ItemLibrary 中随机抽取 1-3 个道具
    static std::default_random_engine rng(std::random_device{}());
    std::uniform_real_distribution<float> offsetDist(-40.0f, 40.0f);
    std::uniform_int_distribution<int> countDist(1, 3);
    
    int dropCount = countDist(rng);
    std::unordered_map<std::string, int> tempOwned = ownedItems;  // 临时计数，避免重复抽到同一个道具
    
    for (int i = 0; i < dropCount; ++i)
    {
        const ItemDef* itemDef = ItemLibrary::pickRandom(tempOwned);
        if (!itemDef) continue;
        
        ItemDrop* drop = ItemDrop::create(itemDef);
        if (drop)
        {
            float offsetX = offsetDist(rng);
            float offsetY = offsetDist(rng);
            
            drop->setPosition(this->getPosition() + Vec2(offsetX, offsetY));
            drop->setGlobalZOrder(Constants::ZOrder::ITEMS);
            
            if (this->getParent())
            {
                this->getParent()->addChild(drop);
            }
            
            drops.pushBack(drop);
            
            // 更新临时计数
            tempOwned[itemDef->id]++;
        }
    }
    
    return drops;
}

// RoomObjectManager 实现
RoomObjectManager::RoomObjectManager()
    : _chest(nullptr)
    , _portal(nullptr)
    , _portalLighting(nullptr)
{
}

RoomObjectManager::~RoomObjectManager()
{
    clear();
}

void RoomObjectManager::createChest(cocos2d::Node* parent, const cocos2d::Vec2& pos)
{
    if (_chest || !parent) return;
    
    _chest = Chest::create();
    if (_chest)
    {
        _chest->setPosition(pos);
        parent->addChild(_chest);
    }
}

bool RoomObjectManager::isChestOpened() const
{
    return _chest ? _chest->isOpened() : true;
}

bool RoomObjectManager::canInteractWithChest(Player* player) const
{
    return _chest ? _chest->canInteract(player) : false;
}

void RoomObjectManager::openChest(Player* player)
{
    if (!_chest || !player) return;
    
    std::unordered_map<std::string, int> ownedItems;
    auto drops = _chest->open(ownedItems);
    for (auto drop : drops)
    {
        _itemDrops.pushBack(drop);
    }
}

bool RoomObjectManager::canInteractWithItemDrop(Player* player) const
{
    if (!player) return false;
    
    for (auto itemDrop : _itemDrops)
    {
        if (itemDrop && itemDrop->canPickup(player))
        {
            return true;
        }
    }
    return false;
}

const ItemDef* RoomObjectManager::pickupItemDrop(Player* player)
{
    if (!player) return nullptr;
    
    for (auto itemDrop : _itemDrops)
    {
        if (itemDrop && itemDrop->canPickup(player))
        {
            const ItemDef* itemDef = itemDrop->pickup(player);
            _itemDrops.eraseObject(itemDrop);
            return itemDef;
        }
    }
    return nullptr;
}

void RoomObjectManager::createPortal(cocos2d::Node* parent, const cocos2d::Vec2& pos)
{
    if (_portal || !parent) return;
    
    _portal = cocos2d::Sprite::create("Map/Portal/Portal_core.png");
    if (_portal)
    {
        _portal->setPosition(pos);
        _portal->setGlobalZOrder(Constants::ZOrder::ITEMS);
        parent->addChild(_portal);
    }
    
    _portalLighting = cocos2d::Sprite::create("Map/Portal/Portal_lighting.png");
    if (_portalLighting)
    {
        _portalLighting->setPosition(pos);
        _portalLighting->setGlobalZOrder(Constants::ZOrder::ITEMS - 1);
        parent->addChild(_portalLighting);
        
        auto fadeOut = cocos2d::FadeOut::create(0.5f);
        auto fadeIn = cocos2d::FadeIn::create(0.5f);
        auto sequence = cocos2d::Sequence::create(fadeOut, fadeIn, nullptr);
        auto repeat = cocos2d::RepeatForever::create(sequence);
        _portalLighting->runAction(repeat);
    }
}

bool RoomObjectManager::canInteractWithPortal(Player* player) const
{
    if (!_portal || !player) return false;
    
    float distance = _portal->getPosition().distance(player->getPosition());
    return distance <= Constants::FLOOR_TILE_SIZE * 2.0f;
}

void RoomObjectManager::clear()
{
    _chest = nullptr;
    _portal = nullptr;
    _portalLighting = nullptr;
    _itemDrops.clear();
}
