#include "GameHUD.h"
#include "Entities/Player/Player.h"
#include "Map/Room.h"
#include "Entities/Objects/Item.h"

GameHUD* GameHUD::create()
{
    GameHUD* ret = new (std::nothrow) GameHUD();
    if (ret && ret->init())
    {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool GameHUD::init()
{
    if (!Node::init())
    {
        return false;
    }
    
    _hpBar = nullptr;
    _mpBar = nullptr;
    _hpIcon = nullptr;
    _mpIcon = nullptr;
    _hpLabel = nullptr;
    _mpLabel = nullptr;
    _skillIcon = nullptr;
    _skillCDProgress = nullptr;
    _skillCDMask = nullptr;
    _healIcon = nullptr;
    _healCDProgress = nullptr;
    _healCDMask = nullptr;
    _debugLabel = nullptr;
    _interactionLabel = nullptr;
    _itemSlots.clear();
    
    createStatusBars();
    createSkillIcons();
    createDebugInfo();
    createControlHints();
    
    return true;
}

void GameHUD::createStatusBars()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();
    
    float barStartX = origin.x + 60;
    float barStartY = origin.y + visibleSize.height - 35;
    float barWidth = 120.0f;
    float barHeight = 12.0f;
    
    // ==================== 血条 ====================
    // 爱心图标
    _hpIcon = Sprite::create("UIs/StatusBars/Bars/Heart.png");
    _hpIcon->setPosition(Vec2(barStartX, barStartY));
    _hpIcon->setScale(0.12f);
    _hpIcon->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL);
    this->addChild(_hpIcon);
    
    // 血条背景
    auto hpBarBg = Sprite::create("UIs/StatusBars/Bars/EmplyBar.png");
    hpBarBg->setPosition(Vec2(barStartX + 25, barStartY));
    hpBarBg->setAnchorPoint(Vec2(0, 0.5f));
    hpBarBg->setScaleX(barWidth / hpBarBg->getContentSize().width);
    hpBarBg->setScaleY(barHeight / hpBarBg->getContentSize().height);
    hpBarBg->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL);
    this->addChild(hpBarBg);
    
    // 血条填充
    _hpBar = cocos2d::ui::LoadingBar::create("UIs/StatusBars/Bars/HealthFill.png");
    _hpBar->setPercent(100.0f);
    _hpBar->setPosition(Vec2(barStartX + 25, barStartY));
    _hpBar->setAnchorPoint(Vec2(0, 0.5f));
    _hpBar->setScaleX(barWidth / _hpBar->getContentSize().width);
    _hpBar->setScaleY(barHeight / _hpBar->getContentSize().height);
    _hpBar->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 5);
    this->addChild(_hpBar);
    
    // 血量数值
    _hpLabel = Label::createWithSystemFont("100/100", "Arial", 16);
    _hpLabel->setPosition(Vec2(barStartX + 75, barStartY));
    _hpLabel->setTextColor(Color4B::WHITE);
    _hpLabel->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 10);
    this->addChild(_hpLabel);
    
    // ==================== 蓝条 ====================
    float mpBarY = barStartY - 35;
    
    // 闪电图标
    _mpIcon = Sprite::create("UIs/StatusBars/Bars/Lighting bolt.png");
    _mpIcon->setPosition(Vec2(barStartX, mpBarY));
    _mpIcon->setScale(0.12f);
    _mpIcon->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL);
    this->addChild(_mpIcon);
    
    // 蓝条背景
    auto mpBarBg = Sprite::create("UIs/StatusBars/Bars/EmplyBar.png");
    mpBarBg->setPosition(Vec2(barStartX + 25, mpBarY));
    mpBarBg->setAnchorPoint(Vec2(0, 0.5f));
    mpBarBg->setScaleX(barWidth / mpBarBg->getContentSize().width);
    mpBarBg->setScaleY(barHeight / mpBarBg->getContentSize().height);
    mpBarBg->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL);
    this->addChild(mpBarBg);
    
    // 蓝条填充
    _mpBar = cocos2d::ui::LoadingBar::create("UIs/StatusBars/Bars/EnrgyFill.png");
    _mpBar->setPercent(100.0f);
    _mpBar->setPosition(Vec2(barStartX + 25, mpBarY));
    _mpBar->setAnchorPoint(Vec2(0, 0.5f));
    _mpBar->setScaleX(barWidth / _mpBar->getContentSize().width);
    _mpBar->setScaleY(barHeight / _mpBar->getContentSize().height);
    _mpBar->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 5);
    this->addChild(_mpBar);
    
    // 蓝量数值
    _mpLabel = Label::createWithSystemFont("100/100", "Arial", 16);
    _mpLabel->setPosition(Vec2(barStartX + 75, mpBarY));
    _mpLabel->setTextColor(Color4B::WHITE);
    _mpLabel->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 10);
    this->addChild(_mpLabel);
}

void GameHUD::createSkillIcons()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();
    
    float skillIconSize = 64.0f;
    float skillIconX = origin.x + visibleSize.width - skillIconSize / 2 - 40;
    float skillIconY = origin.y + skillIconSize / 2 + 40;
    float skillIconY2 = skillIconY + skillIconSize + 10;
    
    // ====== 上方：角色特殊技能（K键） ======
    _skillIcon = Sprite::create("UIs/Skills/Mage/Nymph_skillicon.png");
    _skillIcon->setPosition(Vec2(skillIconX, skillIconY2));
    _skillIcon->setScale(skillIconSize / _skillIcon->getContentSize().width);
    _skillIcon->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL);
    this->addChild(_skillIcon);
    
    // CD变暗遮罩
    _skillCDMask = Sprite::create("UIs/Skills/Mage/Nymph_skillicon.png");
    _skillCDMask->setPosition(Vec2(skillIconX, skillIconY2));
    _skillCDMask->setScale(skillIconSize / _skillCDMask->getContentSize().width);
    _skillCDMask->setColor(Color3B::BLACK);
    _skillCDMask->setOpacity(100);
    _skillCDMask->setVisible(false);
    _skillCDMask->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 1);
    this->addChild(_skillCDMask);
    
    // CD进度条
    auto progressSprite = Sprite::create("UIs/Skills/Mage/Nymph_skillicon.png");
    _skillCDProgress = ProgressTimer::create(progressSprite);
    _skillCDProgress->setType(ProgressTimer::Type::RADIAL);
    _skillCDProgress->setReverseDirection(false);
    _skillCDProgress->setMidpoint(Vec2(0.5f, 0.5f));
    _skillCDProgress->setBarChangeRate(Vec2(1, 1));
    _skillCDProgress->setPosition(Vec2(skillIconX, skillIconY2));
    _skillCDProgress->setScale(skillIconSize / progressSprite->getContentSize().width);
    _skillCDProgress->setPercentage(0.0f);
    _skillCDProgress->setColor(Color3B(100, 100, 100));
    _skillCDProgress->setOpacity(150);
    _skillCDProgress->setVisible(false);
    _skillCDProgress->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 2);
    this->addChild(_skillCDProgress);
    
    // ====== 下方：治疗技能（L键） ======
    _healIcon = Sprite::create("UIs/Skills/Healing.png");
    _healIcon->setPosition(Vec2(skillIconX, skillIconY));
    _healIcon->setScale(skillIconSize / _healIcon->getContentSize().width);
    _healIcon->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL);
    this->addChild(_healIcon);
    
    // 治疗CD变暗遮罩
    _healCDMask = Sprite::create("UIs/Skills/Healing.png");
    _healCDMask->setPosition(Vec2(skillIconX, skillIconY));
    _healCDMask->setScale(skillIconSize / _healCDMask->getContentSize().width);
    _healCDMask->setColor(Color3B::BLACK);
    _healCDMask->setOpacity(100);
    _healCDMask->setVisible(false);
    _healCDMask->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 1);
    this->addChild(_healCDMask);
    
    // 治疗CD进度条
    auto healProgressSprite = Sprite::create("UIs/Skills/Healing.png");
    _healCDProgress = ProgressTimer::create(healProgressSprite);
    _healCDProgress->setType(ProgressTimer::Type::RADIAL);
    _healCDProgress->setReverseDirection(false);
    _healCDProgress->setMidpoint(Vec2(0.5f, 0.5f));
    _healCDProgress->setBarChangeRate(Vec2(1, 1));
    _healCDProgress->setPosition(Vec2(skillIconX, skillIconY));
    _healCDProgress->setScale(skillIconSize / healProgressSprite->getContentSize().width);
    _healCDProgress->setPercentage(0.0f);
    _healCDProgress->setColor(Color3B(100, 100, 100));
    _healCDProgress->setOpacity(150);
    _healCDProgress->setVisible(false);
    _healCDProgress->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 2);
    this->addChild(_healCDProgress);
}

void GameHUD::createDebugInfo()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();
    
    _debugLabel = Label::createWithSystemFont("", "Arial", 18);
    _debugLabel->setPosition(Vec2(origin.x + visibleSize.width - 150, origin.y + visibleSize.height - 30));
    _debugLabel->setTextColor(Color4B::YELLOW);
    _debugLabel->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL);
    this->addChild(_debugLabel);
}

void GameHUD::createControlHints()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();
    
    auto hintLabel = Label::createWithTTF(
        u8"操作说明：\nWASD - 移动\nJ - 攻击\nK - 技能\nL - 治疗\n空格 - 冲刺\nESC - 暂停",
        "fonts/msyh.ttf", 18);
    hintLabel->setPosition(Vec2(origin.x + 120, origin.y + 120));
    hintLabel->setTextColor(Color4B::WHITE);
    hintLabel->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL);
    this->addChild(hintLabel);
}

void GameHUD::update(Player* player, Room* currentRoom, int roomCount)
{
    if (player == nullptr)
    {
        return;
    }
    
    // 玩家死亡时显示HP为0
    int currentHP = player->isDead() ? 0 : player->getHP();
    int maxHP = player->getMaxHP();
    float hpPercent = (maxHP > 0) ? (currentHP * 100.0f / maxHP) : 0.0f;
    _hpBar->setPercent(hpPercent);
    
    char hpText[32];
    sprintf(hpText, "%d/%d", currentHP, maxHP);
    _hpLabel->setString(hpText);
    
    // 更新MP蓝条
    int currentMP = player->getMP();
    int maxMP = player->getMaxMP();
    float mpPercent = (maxMP > 0) ? (currentMP * 100.0f / maxMP) : 0.0f;
    _mpBar->setPercent(mpPercent);
    
    char mpText[32];
    sprintf(mpText, "%d/%d", currentMP, maxMP);
    _mpLabel->setString(mpText);
    
    // 更新技能冷却
    float remain = player->getSkillCooldownRemaining();
    float totalCD = player->getSkillCooldown();
    
    if (remain <= 0.0f)
    {
        _skillIcon->setOpacity(255);
        _skillCDMask->setVisible(false);
        _skillCDProgress->setVisible(false);
    }
    else
    {
        _skillIcon->setOpacity(200);
        _skillCDMask->setVisible(true);
        _skillCDProgress->setVisible(true);
        
        float cdPercent = (totalCD > 0) ? ((totalCD - remain) / totalCD * 100.0f) : 0.0f;
        _skillCDProgress->setPercentage(100.0f - cdPercent);
    }
    
    // 更新治疗技能冷却
    float healRemain = player->getHealCooldownRemaining();
    float healTotalCD = player->getHealCooldown();
    
    if (healRemain <= 0.0f)
    {
        _healIcon->setOpacity(255);
        _healCDMask->setVisible(false);
        _healCDProgress->setVisible(false);
    }
    else
    {
        _healIcon->setOpacity(200);
        _healCDMask->setVisible(true);
        _healCDProgress->setVisible(true);
        
        float healCdPercent = (healTotalCD > 0) ? ((healTotalCD - healRemain) / healTotalCD * 100.0f) : 0.0f;
        _healCDProgress->setPercentage(100.0f - healCdPercent);
    }
    
    // 更新Debug信息
    char debugText[128];
    const char* roomTypeStr = "Unknown";
    if (currentRoom) {
        switch (currentRoom->getRoomType()) {
            case Constants::RoomType::BEGIN: roomTypeStr = "Start"; break;
            case Constants::RoomType::NORMAL: roomTypeStr = "Normal"; break;
            case Constants::RoomType::BOSS: roomTypeStr = "Boss"; break;
            case Constants::RoomType::END: roomTypeStr = "End"; break;
            case Constants::RoomType::REWARD: roomTypeStr = "Reward"; break;
            default: break;
        }
    }
    sprintf(debugText, "Room: %s\nRooms: %d\nPos: (%.0f, %.0f)", 
            roomTypeStr,
            roomCount,
            player->getPositionX(),
            player->getPositionY());
    _debugLabel->setString(debugText);
}

void GameHUD::addItemIcon(const ItemDef* itemDef)
{
    if (!itemDef)
    {
        return;
    }
    
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();
    
    // 道具栏参数
    float barStartX = origin.x + 60;
    float mpBarY = origin.y + visibleSize.height - 70;
    float itemSlotStartX = barStartX - 10;
    float itemSlotStartY = mpBarY - 50;
    float itemSlotSize = 32.0f;
    float itemSlotSpacing = 5.0f;
    int maxItemsPerRow = 5;
    
    // 计算新道具位置
    int itemCount = static_cast<int>(_itemSlots.size());
    int row = itemCount / maxItemsPerRow;
    int col = itemCount % maxItemsPerRow;
    
    float itemX = itemSlotStartX + col * (itemSlotSize + itemSlotSpacing);
    float itemY = itemSlotStartY - row * (itemSlotSize + itemSlotSpacing);
    
    // 创建道具图标
    auto itemIcon = Sprite::create(itemDef->iconPath);
    if (!itemIcon)
    {
        GAME_LOG("Failed to create item icon: %s", itemDef->iconPath.c_str());
        return;
    }
    
    itemIcon->setPosition(Vec2(itemX, itemY));
    itemIcon->setScale(itemSlotSize / itemIcon->getContentSize().width);
    itemIcon->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 5);
    this->addChild(itemIcon);
    
    _itemSlots.push_back(itemIcon);
    
    GAME_LOG("Added item to UI: %s at position (%d, %d)", itemDef->name.c_str(), col, row);
}

void GameHUD::showInteractionHint(const std::string& text, const Vec2& worldPos, float offsetY)
{
    if (!_interactionLabel)
    {
        _interactionLabel = Label::createWithTTF("", "fonts/msyh.ttf", 20);
        _interactionLabel->setTextColor(Color4B::YELLOW);
        _interactionLabel->enableOutline(Color4B::BLACK, 2);
        _interactionLabel->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 5);
        this->addChild(_interactionLabel);
    }
    
    _interactionLabel->setString(text);
    
    // 将世界坐标转换为HUD节点空间坐标
    Vec2 screenPos = this->convertToNodeSpace(worldPos);
    _interactionLabel->setPosition(Vec2(screenPos.x, screenPos.y - offsetY));
    _interactionLabel->setVisible(true);
}

void GameHUD::hideInteractionHint()
{
    if (_interactionLabel && _interactionLabel->isVisible())
    {
        _interactionLabel->setVisible(false);
    }
}
