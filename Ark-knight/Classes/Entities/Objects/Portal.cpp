#include "Portal.h"
#include "Entities/Player/Player.h"
#include "Core/Constants.h"

USING_NS_CC;

Portal* Portal::create()
{
    Portal* portal = new (std::nothrow) Portal();
    if (portal && portal->init())
    {
        portal->autorelease();
        return portal;
    }
    CC_SAFE_DELETE(portal);
    return nullptr;
}

bool Portal::init()
{
    if (!Node::init())
    {
        return false;
    }
    
    _portalSprite = nullptr;
    _lightingSprite = nullptr;
    
    // 创建传送门主体动画（7帧）
    Vector<SpriteFrame*> portalFrames;
    for (int i = 1; i <= 7; i++)
    {
        std::string framePath = "Map/Portal/Portal_000" + std::to_string(i) + ".png";
        auto texture = Director::getInstance()->getTextureCache()->addImage(framePath);
        if (texture)
        {
            auto frame = SpriteFrame::createWithTexture(
                texture, 
                Rect(0, 0, texture->getContentSize().width, texture->getContentSize().height)
            );
            if (frame)
            {
                portalFrames.pushBack(frame);
            }
        }
    }
    
    if (portalFrames.empty())
    {
        CCLOG("Portal: Failed to load portal frames");
        return false;
    }
    
    _portalSprite = Sprite::createWithSpriteFrame(portalFrames.at(0));
    if (!_portalSprite)
    {
        CCLOG("Portal: Failed to create portal sprite");
        return false;
    }
    
    // 缩放到合适大小（3倍地板砖大小）
    float targetSize = Constants::FLOOR_TILE_SIZE * 3.0f;
    float scale = targetSize / _portalSprite->getContentSize().width;
    _portalSprite->setScale(scale);
    _portalSprite->setGlobalZOrder(Constants::ZOrder::FLOOR + 2);
    this->addChild(_portalSprite);
    
    // 播放传送门主体动画（循环）
    auto portalAnimation = Animation::createWithSpriteFrames(portalFrames, 0.1f);
    auto portalAnimate = Animate::create(portalAnimation);
    auto portalRepeat = RepeatForever::create(portalAnimate);
    _portalSprite->runAction(portalRepeat);
    
    // 创建闪电特效动画（4帧）
    Vector<SpriteFrame*> lightingFrames;
    for (int i = 1; i <= 4; i++)
    {
        std::string framePath = "Map/Portal/Portallighting_000" + std::to_string(i) + ".png";
        auto texture = Director::getInstance()->getTextureCache()->addImage(framePath);
        if (texture)
        {
            auto frame = SpriteFrame::createWithTexture(
                texture, 
                Rect(0, 0, texture->getContentSize().width, texture->getContentSize().height)
            );
            if (frame)
            {
                lightingFrames.pushBack(frame);
            }
        }
    }
    
    if (!lightingFrames.empty())
    {
        _lightingSprite = Sprite::createWithSpriteFrame(lightingFrames.at(0));
        if (_lightingSprite)
        {
            _lightingSprite->setScale(scale);
            _lightingSprite->setGlobalZOrder(Constants::ZOrder::FLOOR + 3);
            this->addChild(_lightingSprite);
            
            // 播放闪电动画（循环，速度更快）
            auto lightingAnimation = Animation::createWithSpriteFrames(lightingFrames, 0.08f);
            auto lightingAnimate = Animate::create(lightingAnimation);
            auto lightingRepeat = RepeatForever::create(lightingAnimate);
            _lightingSprite->runAction(lightingRepeat);
        }
    }
    
    return true;
}

bool Portal::canInteract(Player* player, float interactionDistance) const
{
    if (!player)
    {
        return false;
    }
    
    if (interactionDistance <= 0.0f)
    {
        interactionDistance = Constants::FLOOR_TILE_SIZE * 2.5f;
    }
    
    float distance = this->getPosition().distance(player->getPosition());
    return distance <= interactionDistance;
}
