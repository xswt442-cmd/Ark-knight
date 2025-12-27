#include "Chest.h"
#include "Item.h"
#include "ItemDrop.h"
#include "Entities/Player/Player.h"
#include "Core/Constants.h"
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

cocos2d::Vector<ItemDrop*> Chest::open(const std::unordered_map<std::string, int>& ownedItems)
{
    cocos2d::Vector<ItemDrop*> drops;
    
    if (_isOpened)
    {
        return drops;
    }
    
    _isOpened = true;
    
    // 播放打开动画：宝箱慢慢消失（淡出+缩小）
    auto fadeOut = FadeOut::create(0.5f);
    auto scaleDown = ScaleTo::create(0.5f, 0.0f);
    auto spawn = Spawn::create(fadeOut, scaleDown, nullptr);
    auto remove = RemoveSelf::create();
    auto sequence = Sequence::create(spawn, remove, nullptr);
    
    _sprite->runAction(sequence);
    
    // 随机生成 2-4 个道具
    int itemCount = RandomHelper::random_int(2, 4);
    GAME_LOG("Chest opened! Spawning %d items.", itemCount);
    
    for (int i = 0; i < itemCount; ++i) {
        // 抽取道具
        const ItemDef* item = ItemLibrary::pickRandom(ownedItems);
        
        if (!item)
        {
            GAME_LOG("No item available for slot %d!", i);
            continue;
        }
        
        // 创建掉落物
        ItemDrop* drop = ItemDrop::create(item);
        if (drop)
        {
            // 随机分散位置，避免重叠
            float offsetX = RandomHelper::random_real(-40.0f, 40.0f);
            float offsetY = RandomHelper::random_real(-40.0f, 40.0f);
            
            drop->setPosition(this->getPosition() + Vec2(offsetX, offsetY));
            drop->setGlobalZOrder(Constants::ZOrder::FLOOR + 2);
            
            drops.pushBack(drop);
        }
    }
    
    return drops;
}
