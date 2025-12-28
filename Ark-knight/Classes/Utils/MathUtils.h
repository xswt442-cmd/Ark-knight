#ifndef __MATH_UTILS_H__
#define __MATH_UTILS_H__

#include "cocos2d.h"
#include "Core/GameMacros.h"

USING_NS_CC;

// 数学工具类
class MathUtils {
public:
    // 计算两点之间的距离
    static float distance(const Vec2& p1, const Vec2& p2);
    
    // 计算两点之间的角度(弧度)
    static float angle(const Vec2& from, const Vec2& to);
    
    // 计算两点之间的角度(度)
    static float angleDegrees(const Vec2& from, const Vec2& to);
    
    // 向量归一化
    static Vec2 normalize(const Vec2& vec);
    
    // 线性插值
    static float lerp(float from, float to, float t);
    
    // Vec2 线性插值
    static Vec2 lerp(const Vec2& from, const Vec2& to, float t);
    
    // 限制值在范围内
    static float clamp(float value, float min, float max);
    
    // 生成随机整数 [min, max]
    static int randomInt(int min, int max);
    
    // 生成随机浮点数 [min, max]
    static float randomFloat(float min, float max);
    
    // 生成随机布尔值
    static bool randomBool();
    
    // 根据概率返回布尔值 (probability 概率 0.0 ~ 1.0)
    static bool chance(float probability);
};

#endif // __MATH_UTILS_H__
