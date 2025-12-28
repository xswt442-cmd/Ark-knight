#ifndef __GAME_HUD_H__
#define __GAME_HUD_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "Core/Constants.h"
#include "Core/GameMacros.h"

USING_NS_CC;

class Player;
class Room;
struct ItemDef;

// 游戏HUD管理 - 血条、蓝条、技能图标、道具栏等
class GameHUD : public Node {
public:
    static GameHUD* create();
    virtual bool init() override;
    
    // 更新HUD显示
    void update(Player* player, Room* currentRoom, int roomCount);
    
    // 道具栏管理
    void addItemIcon(const ItemDef* itemDef);
    
    // 交互提示显示
    void showInteractionHint(const std::string& text, const Vec2& worldPos, float offsetY);
    
    // 隐藏交互提示
    void hideInteractionHint();
    
    // 获取交互提示标签
    Label* getInteractionLabel() const { return _interactionLabel; }
    
    // 设置技能图标（根据角色类型）
    void setSkillIcon(int characterType);
    
private:
    void createStatusBars();
    void createSkillIcons();
    void createDebugInfo();
    void createControlHints();
    
private:
    // 血条相关
    cocos2d::ui::LoadingBar* _hpBar;
    Sprite* _hpIcon;
    Label* _hpLabel;
    
    // 蓝条相关
    cocos2d::ui::LoadingBar* _mpBar;
    Sprite* _mpIcon;
    Label* _mpLabel;
    
    // 攻击力显示
    Label* _attackLabel;
    
    // 角色技能图标
    Sprite* _skillIcon;
    ProgressTimer* _skillCDProgress;
    Sprite* _skillCDMask;
    
    // 治疗技能图标
    Sprite* _healIcon;
    ProgressTimer* _healCDProgress;
    Sprite* _healCDMask;
    
    // Debug信息
    Label* _debugLabel;
    
    // 交互提示
    Label* _interactionLabel;
    
    // 道具栏
    std::vector<Sprite*> _itemSlots;
};

#endif // __GAME_HUD_H__
