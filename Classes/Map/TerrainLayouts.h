#ifndef __TERRAIN_LAYOUTS_H__
#define __TERRAIN_LAYOUTS_H__

#include "cocos2d.h"
#include "Map/Barriers.h"

// 前置声明
class Room;

/**
 * 普通战斗房间的地形布局类型
 */
enum class TerrainLayout {
    NONE = 0,           // 无特殊地形
    FIVE_BOXES,         // 5堆木箱（左上、左下、右上、右下、中间各3x3）
    NINE_BOXES,         // 9堆木箱（在FIVE_BOXES基础上加左中、上中、右中、下中）
    UPDOWN_SPIKES,      // 矩形围城-上下地刺（左右木箱，上下地刺，四角木箱）
    LEFTRIGHT_SPIKES,   // 矩形围城-左右地刺（上下木箱，左右地刺，四角木箱）
    ALL_SPIKES,         // 矩形围城-一圈地刺（四周地刺，四角也是地刺）
    UPDOWN_BOXES,       // 上下木箱（上下各一排木箱）
    LEFTRIGHT_BOXES,    // 左右木箱（左右各一排木箱）
    CENTER_PILLAR,      // 中心石柱（中间4x4石柱）
    FOUR_PILLARS,       // 4堆石柱（四个角各2x2石柱）
    RANDOM_MESS         // 乱七八糟（随机放置15个石柱+15个木箱）
};

/**
 * TerrainLayoutHelper - 地形布局辅助类
 * 负责在房间中生成各种地形布局
 */
class TerrainLayoutHelper {
public:
    // 应用地形布局到房
    static void applyLayout(Room* room, TerrainLayout layout);
    
private:
    // 各种布局的具体实现
    static void layoutFiveBoxes(Room* room);
    static void layoutNineBoxes(Room* room);
    static void layoutUpDownSpikes(Room* room);
    static void layoutLeftRightSpikes(Room* room);
    static void layoutAllSpikes(Room* room);
    static void layoutUpDownBoxes(Room* room);
    static void layoutLeftRightBoxes(Room* room);
    static void layoutCenterPillar(Room* room);
    static void layoutFourPillars(Room* room);
    static void layoutRandomMess(Room* room);
    
    // 辅助方法：添加指定大小的木箱堆（单堆材质统一）
    static void addBoxCluster(Room* room, int centerTileX, int centerTileY, int width, int height);
    // 辅助方法：添加指定大小的石柱堆（单堆材质统一）
    static void addPillarCluster(Room* room, int centerTileX, int centerTileY, int width, int height);
};

#endif // __TERRAIN_LAYOUTS_H__
