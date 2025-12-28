#pragma once
#include "cocos2d.h"
using namespace cocos2d;

class FloatingText {
public:
    // 显示跳字
    static void show(Node* parent, const Vec2& pos, const std::string& text, const Color3B& color, int fontSize = 20, float duration = 1.2f);
};