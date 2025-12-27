#include "CharacterSelectLayer.h"

// 静态变量初始化，默认选择法师
CharacterType CharacterSelectLayer::s_selectedCharacter = CharacterType::MAGE;

bool CharacterSelectLayer::init()
{
    if (!Layer::init())
    {
        return false;
    }
    
    // 默认选择当前静态变量的值
    _currentSelection = s_selectedCharacter;
    
    _mageSprite = nullptr;
    _gunnerSprite = nullptr;
    _warriorSprite = nullptr;
    _mageLabel = nullptr;
    _gunnerLabel = nullptr;
    _warriorLabel = nullptr;
    _closeCallback = nullptr;
    
    createUI();
    
    return true;
}

void CharacterSelectLayer::createUI()
{
    auto visibleSize = Director::getInstance()->getVisibleSize();
    Vec2 origin = Director::getInstance()->getVisibleOrigin();
    
    // 半透明遮罩
    auto mask = LayerColor::create(Color4B(0, 0, 0, 200));
    mask->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 20);
    this->addChild(mask);
    
    // 标题
    auto titleLabel = Label::createWithTTF(u8"选择角色", "fonts/msyh.ttf", 56);
    titleLabel->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y + 250));
    titleLabel->setTextColor(Color4B::WHITE);
    titleLabel->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 21);
    this->addChild(titleLabel);
    
    // 角色位置计算（左中右）
    float characterY = SCREEN_CENTER.y + 50;
    float spacing = 280.0f;
    float mageX = SCREEN_CENTER.x - spacing;
    float gunnerX = SCREEN_CENTER.x;
    float warriorX = SCREEN_CENTER.x + spacing;
    
    // 角色缩放（使角色看起来差不多大）
    float characterScale = 3.0f;
    
    // ==================== 法师（妮芙）====================
    _mageSprite = Sprite::create("Player/Nymph/Nymph_Idle/Nymph_Idle_0001.png");
    if (_mageSprite)
    {
        _mageSprite->setPosition(Vec2(mageX, characterY));
        _mageSprite->setScale(characterScale);
        _mageSprite->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 22);
        this->addChild(_mageSprite);
        
        // 点击事件
        auto listener = EventListenerTouchOneByOne::create();
        listener->setSwallowTouches(true);
        listener->onTouchBegan = [this](Touch* touch, Event* event) {
            auto target = static_cast<Sprite*>(event->getCurrentTarget());
            Vec2 locationInNode = target->convertToNodeSpace(touch->getLocation());
            Size s = target->getContentSize();
            Rect rect = Rect(0, 0, s.width, s.height);
            if (rect.containsPoint(locationInNode))
            {
                selectCharacter(CharacterType::MAGE);
                return true;
            }
            return false;
        };
        _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, _mageSprite);
    }
    
    _mageLabel = Label::createWithTTF(u8"妮芙\n（法师）", "fonts/msyh.ttf", 24);
    _mageLabel->setPosition(Vec2(mageX, characterY - 120));
    _mageLabel->setAlignment(TextHAlignment::CENTER);
    _mageLabel->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 21);
    this->addChild(_mageLabel);
    
    // ==================== 炮手（维什戴尔）====================
    _gunnerSprite = Sprite::create("Player/Wisdael/Wisdael_Idle/Wisdael_Idle_0001.png");
    if (_gunnerSprite)
    {
        _gunnerSprite->setPosition(Vec2(gunnerX, characterY));
        _gunnerSprite->setScale(characterScale);
        _gunnerSprite->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 22);
        this->addChild(_gunnerSprite);
        
        // 点击事件
        auto listener = EventListenerTouchOneByOne::create();
        listener->setSwallowTouches(true);
        listener->onTouchBegan = [this](Touch* touch, Event* event) {
            auto target = static_cast<Sprite*>(event->getCurrentTarget());
            Vec2 locationInNode = target->convertToNodeSpace(touch->getLocation());
            Size s = target->getContentSize();
            Rect rect = Rect(0, 0, s.width, s.height);
            if (rect.containsPoint(locationInNode))
            {
                selectCharacter(CharacterType::GUNNER);
                return true;
            }
            return false;
        };
        _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, _gunnerSprite);
    }
    
    _gunnerLabel = Label::createWithTTF(u8"维什戴尔\n（炮手）", "fonts/msyh.ttf", 24);
    _gunnerLabel->setPosition(Vec2(gunnerX, characterY - 120));
    _gunnerLabel->setAlignment(TextHAlignment::CENTER);
    _gunnerLabel->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 21);
    this->addChild(_gunnerLabel);
    
    // ==================== 战士（泥岩）====================
    _warriorSprite = Sprite::create("Player/Mudrock/MudRock_Idle/MudRock_Idle_0001.png");
    if (_warriorSprite)
    {
        _warriorSprite->setPosition(Vec2(warriorX, characterY));
        _warriorSprite->setScale(characterScale);
        _warriorSprite->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 22);
        this->addChild(_warriorSprite);
        
        // 点击事件
        auto listener = EventListenerTouchOneByOne::create();
        listener->setSwallowTouches(true);
        listener->onTouchBegan = [this](Touch* touch, Event* event) {
            auto target = static_cast<Sprite*>(event->getCurrentTarget());
            Vec2 locationInNode = target->convertToNodeSpace(touch->getLocation());
            Size s = target->getContentSize();
            Rect rect = Rect(0, 0, s.width, s.height);
            if (rect.containsPoint(locationInNode))
            {
                selectCharacter(CharacterType::WARRIOR);
                return true;
            }
            return false;
        };
        _eventDispatcher->addEventListenerWithSceneGraphPriority(listener, _warriorSprite);
    }
    
    _warriorLabel = Label::createWithTTF(u8"泥岩\n（战士）", "fonts/msyh.ttf", 24);
    _warriorLabel->setPosition(Vec2(warriorX, characterY - 120));
    _warriorLabel->setAlignment(TextHAlignment::CENTER);
    _warriorLabel->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 21);
    this->addChild(_warriorLabel);
    
    // ==================== 确认按钮 ====================
    auto confirmButton = ui::Button::create();
    confirmButton->setTitleText(u8"确认");
    confirmButton->setTitleFontName("fonts/msyh.ttf");
    confirmButton->setTitleFontSize(36);
    confirmButton->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y - 200));
    confirmButton->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 21);
    confirmButton->addClickEventListener([this](Ref* sender) {
        confirmSelection();
    });
    this->addChild(confirmButton);
    
    // ==================== 返回按钮 ====================
    auto backButton = ui::Button::create();
    backButton->setTitleText(u8"返回");
    backButton->setTitleFontName("fonts/msyh.ttf");
    backButton->setTitleFontSize(28);
    backButton->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y - 260));
    backButton->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 21);
    backButton->addClickEventListener([this](Ref* sender) {
        // 不保存，直接关闭
        if (_closeCallback)
        {
            _closeCallback();
        }
        this->removeFromParent();
    });
    this->addChild(backButton);
    
    // 更新显示状态
    updateCharacterDisplay();
    
    GAME_LOG("CharacterSelectLayer created");
}

void CharacterSelectLayer::selectCharacter(CharacterType type)
{
    _currentSelection = type;
    updateCharacterDisplay();
    
    const char* names[] = {"Mage", "Gunner", "Warrior"};
    GAME_LOG("Character selected: %s", names[static_cast<int>(type)]);
}

void CharacterSelectLayer::updateCharacterDisplay()
{
    // 选中的角色高亮（白色），其他变暗（灰色）
    Color3B selectedColor = Color3B::WHITE;
    Color3B unselectedColor = Color3B(100, 100, 100);
    
    // 法师
    if (_mageSprite)
    {
        _mageSprite->setColor(_currentSelection == CharacterType::MAGE ? selectedColor : unselectedColor);
    }
    if (_mageLabel)
    {
        _mageLabel->setTextColor(_currentSelection == CharacterType::MAGE ? Color4B::WHITE : Color4B(100, 100, 100, 255));
    }
    
    // 炮手
    if (_gunnerSprite)
    {
        _gunnerSprite->setColor(_currentSelection == CharacterType::GUNNER ? selectedColor : unselectedColor);
    }
    if (_gunnerLabel)
    {
        _gunnerLabel->setTextColor(_currentSelection == CharacterType::GUNNER ? Color4B::WHITE : Color4B(100, 100, 100, 255));
    }
    
    // 战士
    if (_warriorSprite)
    {
        _warriorSprite->setColor(_currentSelection == CharacterType::WARRIOR ? selectedColor : unselectedColor);
    }
    if (_warriorLabel)
    {
        _warriorLabel->setTextColor(_currentSelection == CharacterType::WARRIOR ? Color4B::WHITE : Color4B(100, 100, 100, 255));
    }
}

void CharacterSelectLayer::confirmSelection()
{
    // 保存选择到静态变量
    s_selectedCharacter = _currentSelection;
    
    const char* names[] = {"Mage", "Gunner", "Warrior"};
    GAME_LOG("Character confirmed: %s", names[static_cast<int>(_currentSelection)]);
    
    // 调用回调
    if (_closeCallback)
    {
        _closeCallback();
    }
    
    // 移除自己
    this->removeFromParent();
}
