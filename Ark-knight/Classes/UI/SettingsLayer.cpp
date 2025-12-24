#include "SettingsLayer.h"

bool SettingsLayer::init()
{
    if (!Layer::init())
    {
        return false;
    }
    
    createUI();
    
    return true;
}

void SettingsLayer::createUI()
{
    // 半透明遮罩
    auto mask = LayerColor::create(Color4B(0, 0, 0, 180));
    this->addChild(mask);
    
    // 设置标题
    auto settingsTitle = Label::createWithTTF(u8"设置", "fonts/msyh.ttf", 56);
    settingsTitle->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y + 200));
    settingsTitle->setTextColor(Color4B::WHITE);
    this->addChild(settingsTitle);
    
    // 分辨率标签
    auto resLabel = Label::createWithTTF(u8"分辨率：", "fonts/msyh.ttf", 32);
    resLabel->setPosition(Vec2(SCREEN_CENTER.x - 200, SCREEN_CENTER.y + 100));
    resLabel->setTextColor(Color4B::WHITE);
    this->addChild(resLabel);
    
    // 分辨率选项按钮
    const char* resolutions[] = {u8"800x600", u8"1280x720", u8"1920x1080"};
    int resWidth[] = {800, 1280, 1920};
    int resHeight[] = {600, 720, 1080};
    
    for (int i = 0; i < 3; i++)
    {
        auto resButton = ui::Button::create();
        resButton->setTitleText(resolutions[i]);
        resButton->setTitleFontName("fonts/msyh.ttf");
        resButton->setTitleFontSize(28);
        resButton->setPosition(Vec2(SCREEN_CENTER.x + 80 + i * 200, SCREEN_CENTER.y + 100));
        int width = resWidth[i];
        int height = resHeight[i];
        resButton->addClickEventListener([width, height](Ref* sender) {
            auto director = Director::getInstance();
            auto glview = director->getOpenGLView();
            glview->setFrameSize(width, height);
            glview->setDesignResolutionSize(Constants::DESIGN_WIDTH, Constants::DESIGN_HEIGHT, ResolutionPolicy::SHOW_ALL);
            GAME_LOG("Resolution changed to %dx%d", width, height);
        });
        this->addChild(resButton);
    }
    
    // 返回按钮
    auto backButton = ui::Button::create();
    backButton->setTitleText(u8"返回");
    backButton->setTitleFontName("fonts/msyh.ttf");
    backButton->setTitleFontSize(32);
    backButton->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y - 100));
    backButton->addClickEventListener([this](Ref* sender) {
        close();
    });
    this->addChild(backButton);
    
    // 设置全局Z顺序
    this->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 10);
}

void SettingsLayer::close()
{
    GAME_LOG("Closing settings layer");
    
    if (_closeCallback)
    {
        _closeCallback();
    }
    
    this->removeFromParent();
}
