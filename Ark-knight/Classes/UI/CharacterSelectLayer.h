#ifndef __CHARACTER_SELECT_LAYER_H__
#define __CHARACTER_SELECT_LAYER_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "Core/Constants.h"
#include "Core/GameMacros.h"

USING_NS_CC;

/**
 * 角色类型枚举
 */
enum class CharacterType {
    MAGE = 0,      // 法师（妮芙）
    GUNNER = 1,    // 炮手（维什戴尔）
    WARRIOR = 2    // 战士（泥岩）
};

/**
 * 角色选择界面Layer
 * 显示三个角色供玩家选择
 */
class CharacterSelectLayer : public Layer {
public:
    virtual bool init() override;
    
    CREATE_FUNC(CharacterSelectLayer);
    
    /**
     * 设置关闭回调
     */
    void setCloseCallback(const std::function<void()>& callback) {
        _closeCallback = callback;
    }
    
    /**
     * 获取当前选择的角色类型
     */
    static CharacterType getSelectedCharacter() { return s_selectedCharacter; }
    
    /**
     * 设置选择的角色类型
     */
    static void setSelectedCharacter(CharacterType type) { s_selectedCharacter = type; }
    
private:
    /**
     * 创建UI元素
     */
    void createUI();
    
    /**
     * 选择角色
     */
    void selectCharacter(CharacterType type);
    
    /**
     * 确认选择并关闭
     */
    void confirmSelection();
    
    /**
     * 更新角色显示状态（选中高亮，其他变暗）
     */
    void updateCharacterDisplay();
    
private:
    // 静态变量保存选择的角色
    static CharacterType s_selectedCharacter;
    
    // 当前选择的角色（本地）
    CharacterType _currentSelection;
    
    // 角色图片精灵
    Sprite* _mageSprite;
    Sprite* _gunnerSprite;
    Sprite* _warriorSprite;
    
    // 角色名称标签
    Label* _mageLabel;
    Label* _gunnerLabel;
    Label* _warriorLabel;
    
    // 回调
    std::function<void()> _closeCallback;
};

#endif // __CHARACTER_SELECT_LAYER_H__
