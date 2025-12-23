#pragma once
#include "cocos2d.h"
using namespace cocos2d;

class FloatingText {
public:
    // 在 parent 节点上 world/local 坐标 pos 显示文本，颜色 Color3B，文本大小 fontSize，显示时长 duration（秒）
    static void show(Node* parent, const Vec2& pos, const std::string& text, const Color3B& color, int fontSize = 20, float duration = 1.2f);
};