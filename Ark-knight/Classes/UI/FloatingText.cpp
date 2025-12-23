#include "FloatingText.h"
#include "Core/Constants.h"

USING_NS_CC;

namespace {
    // 仅记录当前活跃的浮动文本数量（避免保存裸指针导致悬挂指针访问）
    static size_t s_activeCount = 0;
    static const size_t MAX_ACTIVE = 15;
}

void FloatingText::show(Node* parent, const Vec2& pos, const std::string& text, const Color3B& color, int fontSize, float duration)
{
    if (parent == nullptr) return;

    // 限制同时显示数量，达到上限则忽略新的（避免卡顿）
    if (s_activeCount >= MAX_ACTIVE)
    {
        return;
    }

    // 创建Label（使用项目内字体或系统字体回退）
    Label* label = Label::createWithTTF(text, "fonts/msyh.ttf", fontSize);
    if (!label)
    {
        label = Label::createWithSystemFont(text, "Arial", fontSize);
    }
    if (!label) return;

    label->setTextColor(Color4B(color));
    label->setAnchorPoint(Vec2(0.5f, 0.5f));

    // pos 假设为添加到 parent 的局部坐标（通常 entity->getPosition() 即可）
    label->setPosition(pos);
    label->setGlobalZOrder(Constants::ZOrder::EFFECT + 1);

    parent->addChild(label);

    // 增加活跃计数
    ++s_activeCount;

    // 动作：向上移动并渐隐，然后移除并更新计数
    Vec2 moveBy = Vec2(0, 30.0f);
    float fadeTime = duration * 0.6f;
    float totalTime = duration;

    auto move = MoveBy::create(totalTime, moveBy);
    auto fade = FadeOut::create(fadeTime);
    auto delay = DelayTime::create(totalTime - fadeTime);
    auto seq = Sequence::create(delay, fade, nullptr);
    auto spawn = Spawn::createWithTwoActions(move, seq);

    // 完成回调：移除 label 并递减活跃计数（小心不要越界）
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