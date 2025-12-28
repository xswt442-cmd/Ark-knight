#ifndef __GAME_MACROS_H__
#define __GAME_MACROS_H__

#include "cocos2d.h"

// 全局宏定义和工具枚举

// 便捷宏
#define SCREEN_SIZE Director::getInstance()->getVisibleSize()
#define SCREEN_ORIGIN Director::getInstance()->getVisibleOrigin()
#define SCREEN_WIDTH SCREEN_SIZE.width
#define SCREEN_HEIGHT SCREEN_SIZE.height
#define SCREEN_CENTER Vec2(SCREEN_ORIGIN.x + SCREEN_WIDTH / 2, SCREEN_ORIGIN.y + SCREEN_HEIGHT / 2)

// 安全删除宏
#define SAFE_DELETE(p) do { if(p) { delete (p); (p) = nullptr; } } while(0)
#define SAFE_RELEASE(p) do { if(p) { (p)->release(); (p) = nullptr; } } while(0)

// 物理碰撞掩码
namespace PhysicsMask {
    constexpr int NONE        = 0x00000000;
    constexpr int PLAYER      = 0x00000001;   // 0001
    constexpr int ENEMY       = 0x00000002;   // 0010
    constexpr int PROJECTILE  = 0x00000004;   // 0100
    constexpr int OBSTACLE    = 0x00000008;   // 1000
    constexpr int PROP        = 0x00000010;   // 10000
    constexpr int ALL         = 0xFFFFFFFF;
}

// 实体状态枚举
enum class EntityState {
    IDLE,
    MOVE,
    ATTACK,
    SKILL,
    HIT,
    DASH,
    DIE
};

// 房间类型枚举
enum class RoomType {
    NORMAL,      // 普通战斗房间
    BOSS,        // Boss房间
    TREASURE,    // 奖励房间
    SHOP,        // 商店房间
    START        // 起始房间
};

// 方向枚举
enum class Direction {
    NONE = -1,
    UP = 0,
    DOWN = 1,
    LEFT = 2,
    RIGHT = 3
};

// 武器类型枚举
enum class WeaponType {
    MELEE,       // 近战
    RANGED,      // 远程
    MAGIC        // 魔法
};

// 敌人类型枚举
enum class EnemyType {
    MELEE,       // 近战
    RANGED,      // 远程
    BOSS         // Boss
};

// 数学工具宏
#define DEG_TO_RAD(deg) ((deg) * M_PI / 180.0f)
#define RAD_TO_DEG(rad) ((rad) * 180.0f / M_PI)

// 随机数宏
#define RANDOM_INT(min, max) (cocos2d::RandomHelper::random_int(min, max))
#define RANDOM_FLOAT(min, max) (cocos2d::RandomHelper::random_real<float>(min, max))

// 日志宏
#ifdef COCOS2D_DEBUG
    #define GAME_LOG(format, ...) cocos2d::log("[GAME] " format, ##__VA_ARGS__)
    #define GAME_LOG_ERROR(format, ...) cocos2d::log("[ERROR] " format, ##__VA_ARGS__)
#else
    #define GAME_LOG(format, ...)
    #define GAME_LOG_ERROR(format, ...)
#endif

#endif // __GAME_MACROS_H__
