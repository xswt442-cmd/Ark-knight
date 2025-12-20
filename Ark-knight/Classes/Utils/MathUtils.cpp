#include "MathUtils.h"
#include <cmath>

float MathUtils::distance(const Vec2& p1, const Vec2& p2)
{
    return p1.distance(p2);
}

float MathUtils::angle(const Vec2& from, const Vec2& to)
{
    Vec2 direction = to - from;
    return static_cast<float>(std::atan2(direction.y, direction.x));
}

float MathUtils::angleDegrees(const Vec2& from, const Vec2& to)
{
    return RAD_TO_DEG(angle(from, to));
}

Vec2 MathUtils::normalize(const Vec2& vec)
{
    float length = vec.length();
    if (length < 0.0001f)
    {
        return Vec2::ZERO;
    }
    return vec / length;
}

float MathUtils::lerp(float from, float to, float t)
{
    t = clamp(t, 0.0f, 1.0f);
    return from + (to - from) * t;
}

Vec2 MathUtils::lerp(const Vec2& from, const Vec2& to, float t)
{
    t = clamp(t, 0.0f, 1.0f);
    return from + (to - from) * t;
}

float MathUtils::clamp(float value, float min, float max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

int MathUtils::randomInt(int min, int max)
{
    return RANDOM_INT(min, max);
}

float MathUtils::randomFloat(float min, float max)
{
    return RANDOM_FLOAT(min, max);
}

bool MathUtils::randomBool()
{
    return RANDOM_INT(0, 1) == 1;
}

bool MathUtils::chance(float probability)
{
    return RANDOM_FLOAT(0.0f, 1.0f) < probability;
}
