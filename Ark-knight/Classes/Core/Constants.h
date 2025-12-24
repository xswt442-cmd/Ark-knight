#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#include <string>

/**
 * 全局常量定义
 * 包含游戏中使用的所有常量值
 */

namespace Constants {
    // ==================== 游戏配置 ====================
    constexpr float DESIGN_WIDTH = 1280.0f;
    constexpr float DESIGN_HEIGHT = 720.0f;
    
    // ==================== 玩家属性 ====================
    namespace Player {
        constexpr int DEFAULT_HP = 100;
        constexpr int DEFAULT_MP = 100;
        constexpr float DEFAULT_MOVE_SPEED = 200.0f;
        constexpr float DASH_SPEED = 400.0f;
        constexpr float DASH_DURATION = 0.3f;
    }
    
    // ==================== 敌人属性 ====================
    namespace Enemy {
        constexpr int MELEE_HP = 50;
        constexpr int RANGED_HP = 30;
        constexpr int BOSS_HP = 500;
        constexpr float DEFAULT_MOVE_SPEED = 100.0f;
        constexpr float CHASE_RANGE = 300.0f;
        constexpr float ATTACK_RANGE = 50.0f;
    }
    
    // ==================== 地图配置 ====================
    // 注意：不要用Map作为命名空间名，会和cocos2d::Map冲突
    constexpr int MAP_MAX_ROOMS = 6;           // 最大房间数量
    constexpr int MAP_GRID_SIZE = 5;           // 5x5 地图矩阵
    constexpr float FLOOR_TILE_SIZE = 32.0f;   // 地板尺寸
    constexpr int ROOM_TILES_W = 26;           // 房间宽度(地板块数) - 偶数
    constexpr int ROOM_TILES_H = 18;           // 房间高度(地板块数) - 偶数
    constexpr float ROOM_CENTER_DIST = 960.0f; // 房间中心距离
    constexpr int DOOR_WIDTH = 4;              // 门宽度(地板块数) - 偶数
    
    // ==================== 方向定义 ====================
    constexpr int DIR_UP = 0;
    constexpr int DIR_RIGHT = 1;
    constexpr int DIR_DOWN = 2;
    constexpr int DIR_LEFT = 3;
    constexpr int DIR_COUNT = 4;
    // 方向偏移在cpp中定义
    
    // ==================== 战斗配置 ====================
    namespace Combat {
        constexpr float ATTACK_COOLDOWN = 0.5f;
        constexpr float SKILL_COOLDOWN = 3.0f;
        constexpr int PROJECTILE_SPEED = 500;
    }
    
    // ==================== 资源路径 ====================
    namespace Path {
        // 场景
        const std::string SCENE_MENU = "scenes/menu.png";
        const std::string SCENE_GAME = "scenes/game_bg.png";
        
        // 玩家
        const std::string PLAYER_MAGE = "player/mage.png";
        const std::string PLAYER_WARRIOR = "player/warrior.png";
        const std::string PLAYER_ALCHEMIST = "player/alchemist.png";
        
        // 敌人
        const std::string ENEMY_MELEE = "enemy/melee.png";
        const std::string ENEMY_RANGED = "enemy/ranged.png";
        const std::string ENEMY_BOSS = "enemy/boss.png";
        
        // UI
        const std::string UI_HP_BAR = "ui/hp_bar.png";
        const std::string UI_MP_BAR = "ui/mp_bar.png";
        
        // 音频
        const std::string BGM_MENU = "audio/bgm_menu.mp3";
        const std::string BGM_BATTLE = "audio/bgm_battle.mp3";
        const std::string BGM_BOSS = "audio/bgm_boss.mp3";
        
        const std::string SFX_ATTACK = "audio/sfx_attack.wav";
        const std::string SFX_SKILL = "audio/sfx_skill.wav";
        const std::string SFX_HIT = "audio/sfx_hit.wav";
    }
    
    // ==================== 游戏标签 ====================
    namespace Tag {
        constexpr int PLAYER = 100;
        constexpr int ENEMY = 200;
        constexpr int PROJECTILE = 300;
        constexpr int PROP = 400;
        constexpr int OBSTACLE = 500;
        constexpr int WALL = 600;
    }
    
    // ==================== Z-Order ====================
    namespace ZOrder {
        constexpr int BACKGROUND = 0;
        constexpr int FLOOR = 10;
        constexpr int DOOR = 12;
        constexpr int WALL_BELOW = 15;     // 玩家下方的墙（上方墙壁）
        constexpr int PROP = 20;
        constexpr int SHADOW = 25;
        constexpr int ENTITY = 50;         // 玩家和敌人
        constexpr int PROJECTILE = 55;
        constexpr int WALL_ABOVE = 60;     // 玩家上方的墙（左右下墙壁）
        constexpr int EFFECT = 70;
        constexpr int MINIMAP = 90;
        constexpr int UI = 100;
        constexpr int UI_GLOBAL = 500;     // UI元素的全局Z序（确保不被遮挡）
    }
    
    // ==================== 房间类型 ====================
    enum class RoomType {
        NONE,       // 无房间
        BEGIN,      // 起始房间
        NORMAL,     // 普通战斗房间
        BOSS,       // Boss房间
        END,        // 终点房间(下一关传送门)
        REWARD      // 奖励房间（原武器/道具房间）
    };
}

#endif // __CONSTANTS_H__
