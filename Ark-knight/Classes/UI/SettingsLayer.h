#ifndef __SETTINGS_LAYER_H__
#define __SETTINGS_LAYER_H__

#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "Core/Constants.h"
#include "Core/GameMacros.h"

USING_NS_CC;

/**
 * 设置界面Layer
 * 可被任何场景复用的设置界面
 */
class SettingsLayer : public Layer {
public:
    virtual bool init() override;
    
    CREATE_FUNC(SettingsLayer);
    
    /**
     * 设置关闭回调
     */
    void setCloseCallback(const std::function<void()>& callback) {
        _closeCallback = callback;
    }
    
private:
    /**
     * 创建UI元素
     */
    void createUI();
    
    /**
     * 关闭设置界面
     */
    void close();
    
    std::function<void()> _closeCallback;
};

#endif // __SETTINGS_LAYER_H__
