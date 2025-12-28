#include "FloatingText.h"
#include "Core/Constants.h"

/*
 *本方法实现跳字功能，包括伤害数字、治疗数字、战士护盾的抵挡标志文字等
 */

USING_NS_CC;

namespace {
    static size_t s_activeCount = 0;
    static const size_t MAX_ACTIVE = 15;
}

void FloatingText::show(Node* parent, const Vec2& pos, const std::string& text, const Color3B& color, int fontSize, float duration)
{
    if (parent == nullptr) return;

    if (s_activeCount >= MAX_ACTIVE)
    {
        return;
    }

    Label* label = Label::createWithTTF(text, "fonts/msyh.ttf", fontSize);
    if (!label)
    {
        label = Label::createWithSystemFont(text, "Arial", fontSize);
    }
    if (!label) return;

    label->setTextColor(Color4B(color));
    label->setAnchorPoint(Vec2(0.5f, 0.5f));

    label->setPosition(pos);
    label->setGlobalZOrder(Constants::ZOrder::EFFECT + 1);

    parent->addChild(label);

    ++s_activeCount;

    Vec2 moveBy = Vec2(0, 30.0f);
    float fadeTime = duration * 0.6f;
    float totalTime = duration;

    auto move = MoveBy::create(totalTime, moveBy);
    auto fade = FadeOut::create(fadeTime);
    auto delay = DelayTime::create(totalTime - fadeTime);
    auto seq = Sequence::create(delay, fade, nullptr);
    auto spawn = Spawn::createWithTwoActions(move, seq);

    auto cleanup = CallFunc::create([label]() {
        if (label->getParent())
        {
            label->removeFromParent();
        }
        if (s_activeCount > 0)
        {
            --s_activeCount;
        }
    });

    auto full = Sequence::create(spawn, cleanup, nullptr);
    label->runAction(full);
}