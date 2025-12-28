#include "SettingsLayer.h"
#include "Managers/SoundManager.h"

bool SettingsLayer::init()
{
    if (!Layer::init())
    {
        return false;
    }
    
    _bgmVolumeLabel = nullptr;
    _sfxVolumeLabel = nullptr;
    
    createUI();
    
    return true;
}

void SettingsLayer::createUI()
{
    // 半透明遮罩
    auto mask = LayerColor::create(Color4B(0, 0, 0, 180));
    mask->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 10);
    this->addChild(mask);
    
    // 设置标题
    auto settingsTitle = Label::createWithTTF(u8"设置", "fonts/msyh.ttf", 56);
    settingsTitle->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y + 200));
    settingsTitle->setTextColor(Color4B::WHITE);
    settingsTitle->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 11);
    this->addChild(settingsTitle);
    
    // 创建音量滑块
    createVolumeSliders();
    
    // 分辨率标签
    auto resLabel = Label::createWithTTF(u8"分辨率：", "fonts/msyh.ttf", 32);
    resLabel->setPosition(Vec2(SCREEN_CENTER.x * 0.5f, SCREEN_CENTER.y - 20));
    resLabel->setTextColor(Color4B::WHITE);
    resLabel->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 11);
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
        resButton->setPosition(Vec2(SCREEN_CENTER.x + i * 180 - 120, SCREEN_CENTER.y - 20));
        resButton->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 11);
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
    backButton->setPosition(Vec2(SCREEN_CENTER.x, SCREEN_CENTER.y - 150));
    backButton->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 11);
    backButton->addClickEventListener([this](Ref* sender) {
        close();
    });
    this->addChild(backButton);
}

void SettingsLayer::createVolumeSliders()
{
    auto soundManager = SoundManager::getInstance();
    float sliderWidth = 300.0f;
    
    // ========== 背景音乐音量 ==========
    // 标签
    auto bgmLabel = Label::createWithTTF(u8"背景音乐：", "fonts/msyh.ttf", 32);
    bgmLabel->setPosition(Vec2(SCREEN_CENTER.x - 200, SCREEN_CENTER.y + 120));
    bgmLabel->setAnchorPoint(Vec2(1.0f, 0.5f));
    bgmLabel->setTextColor(Color4B::WHITE);
    bgmLabel->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 11);
    this->addChild(bgmLabel);
    
    // 滑块
    auto bgmSlider = ui::Slider::create();
    bgmSlider->loadBarTexture("UI/slider_bar.png");
    bgmSlider->loadSlidBallTextures("UI/slider_ball.png", "UI/slider_ball.png", "");
    bgmSlider->loadProgressBarTexture("UI/slider_progress.png");
    
    // 如果没有图片资源，使用程序化生成
    if (bgmSlider->getContentSize().width == 0)
    {
        // 创建简单的滑块背景
        auto sliderBg = DrawNode::create();
        sliderBg->drawSolidRect(Vec2(0, -5), Vec2(sliderWidth, 5), Color4F(0.3f, 0.3f, 0.3f, 1.0f));
        sliderBg->setPosition(Vec2(SCREEN_CENTER.x - 50, SCREEN_CENTER.y + 120));
        sliderBg->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 11);
        this->addChild(sliderBg);
        
        // 使用按钮模拟滑块 - 减少
        auto bgmMinus = ui::Button::create();
        bgmMinus->setTitleText("-");
        bgmMinus->setTitleFontName("fonts/msyh.ttf");
        bgmMinus->setTitleFontSize(32);
        bgmMinus->setPosition(Vec2(SCREEN_CENTER.x - 80, SCREEN_CENTER.y + 120));
        bgmMinus->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 11);
        bgmMinus->addClickEventListener([this](Ref* sender) {
            auto sm = SoundManager::getInstance();
            float vol = sm->getBGMVolume() - 0.1f;
            if (vol < 0.0f) vol = 0.0f;
            sm->setBGMVolume(vol);
            char buf[32];
            sprintf(buf, "%d%%", (int)(vol * 100));
            if (_bgmVolumeLabel) _bgmVolumeLabel->setString(buf);
        });
        this->addChild(bgmMinus);
        
        // 音量数值显示
        _bgmVolumeLabel = Label::createWithTTF("", "fonts/msyh.ttf", 28);
        char bgmBuf[32];
        sprintf(bgmBuf, "%d%%", (int)(soundManager->getBGMVolume() * 100));
        _bgmVolumeLabel->setString(bgmBuf);
        _bgmVolumeLabel->setPosition(Vec2(SCREEN_CENTER.x + 50, SCREEN_CENTER.y + 120));
        _bgmVolumeLabel->setTextColor(Color4B::WHITE);
        _bgmVolumeLabel->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 11);
        this->addChild(_bgmVolumeLabel);
        
        // 使用按钮模拟滑块 - 增加
        auto bgmPlus = ui::Button::create();
        bgmPlus->setTitleText("+");
        bgmPlus->setTitleFontName("fonts/msyh.ttf");
        bgmPlus->setTitleFontSize(32);
        bgmPlus->setPosition(Vec2(SCREEN_CENTER.x + 180, SCREEN_CENTER.y + 120));
        bgmPlus->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 11);
        bgmPlus->addClickEventListener([this](Ref* sender) {
            auto sm = SoundManager::getInstance();
            float vol = sm->getBGMVolume() + 0.1f;
            if (vol > 1.0f) vol = 1.0f;
            sm->setBGMVolume(vol);
            char buf[32];
            sprintf(buf, "%d%%", (int)(vol * 100));
            if (_bgmVolumeLabel) _bgmVolumeLabel->setString(buf);
        });
        this->addChild(bgmPlus);
    }
    
    // ========== 音效音量 ==========
    // 标签
    auto sfxLabel = Label::createWithTTF(u8"音效音量：", "fonts/msyh.ttf", 32);
    sfxLabel->setPosition(Vec2(SCREEN_CENTER.x - 200, SCREEN_CENTER.y + 50));
    sfxLabel->setAnchorPoint(Vec2(1.0f, 0.5f));
    sfxLabel->setTextColor(Color4B::WHITE);
    sfxLabel->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 11);
    this->addChild(sfxLabel);
    
    // 创建简单的滑块背景
    auto sfxSliderBg = DrawNode::create();
    sfxSliderBg->drawSolidRect(Vec2(0, -5), Vec2(sliderWidth, 5), Color4F(0.3f, 0.3f, 0.3f, 1.0f));
    sfxSliderBg->setPosition(Vec2(SCREEN_CENTER.x - 50, SCREEN_CENTER.y + 50));
    sfxSliderBg->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 11);
    this->addChild(sfxSliderBg);
    
    // 使用按钮模拟滑块 - 减少
    auto sfxMinus = ui::Button::create();
    sfxMinus->setTitleText("-");
    sfxMinus->setTitleFontName("fonts/msyh.ttf");
    sfxMinus->setTitleFontSize(32);
    sfxMinus->setPosition(Vec2(SCREEN_CENTER.x - 80, SCREEN_CENTER.y + 50));
    sfxMinus->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 11);
    sfxMinus->addClickEventListener([this](Ref* sender) {
        auto sm = SoundManager::getInstance();
        float vol = sm->getSFXVolume() - 0.1f;
        if (vol < 0.0f) vol = 0.0f;
        sm->setSFXVolume(vol);
        char buf[32];
        sprintf(buf, "%d%%", (int)(vol * 100));
        if (_sfxVolumeLabel) _sfxVolumeLabel->setString(buf);
        // 播放测试音效
        sm->playSFX("SoundEffect/MudRock_Attack.mp3");
    });
    this->addChild(sfxMinus);
    
    // 音量数值显示
    _sfxVolumeLabel = Label::createWithTTF("", "fonts/msyh.ttf", 28);
    char sfxBuf[32];
    sprintf(sfxBuf, "%d%%", (int)(soundManager->getSFXVolume() * 100));
    _sfxVolumeLabel->setString(sfxBuf);
    _sfxVolumeLabel->setPosition(Vec2(SCREEN_CENTER.x + 50, SCREEN_CENTER.y + 50));
    _sfxVolumeLabel->setTextColor(Color4B::WHITE);
    _sfxVolumeLabel->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 11);
    this->addChild(_sfxVolumeLabel);
    
    // 使用按钮模拟滑块 - 增加
    auto sfxPlus = ui::Button::create();
    sfxPlus->setTitleText("+");
    sfxPlus->setTitleFontName("fonts/msyh.ttf");
    sfxPlus->setTitleFontSize(32);
    sfxPlus->setPosition(Vec2(SCREEN_CENTER.x + 180, SCREEN_CENTER.y + 50));
    sfxPlus->setGlobalZOrder(Constants::ZOrder::UI_GLOBAL + 11);
    sfxPlus->addClickEventListener([this](Ref* sender) {
        auto sm = SoundManager::getInstance();
        float vol = sm->getSFXVolume() + 0.1f;
        if (vol > 1.0f) vol = 1.0f;
        sm->setSFXVolume(vol);
        char buf[32];
        sprintf(buf, "%d%%", (int)(vol * 100));
        if (_sfxVolumeLabel) _sfxVolumeLabel->setString(buf);
        // 播放测试音效
        sm->playSFX("SoundEffect/MudRock_Attack.mp3");
    });
    this->addChild(sfxPlus);
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
