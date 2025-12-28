#include "TerrainLayouts.h"
#include "Room.h"
#include "Core/Constants.h"

USING_NS_CC;

void TerrainLayoutHelper::applyLayout(Room* room, TerrainLayout layout)
{
    if (!room) return;
    
    switch (layout)
    {
    case TerrainLayout::FIVE_BOXES:
        layoutFiveBoxes(room);
        break;
    case TerrainLayout::NINE_BOXES:
        layoutNineBoxes(room);
        break;
    case TerrainLayout::UPDOWN_SPIKES:
        layoutUpDownSpikes(room);
        break;
    case TerrainLayout::LEFTRIGHT_SPIKES:
        layoutLeftRightSpikes(room);
        break;
    case TerrainLayout::ALL_SPIKES:
        layoutAllSpikes(room);
        break;
    case TerrainLayout::UPDOWN_BOXES:
        layoutUpDownBoxes(room);
        break;
    case TerrainLayout::LEFTRIGHT_BOXES:
        layoutLeftRightBoxes(room);
        break;
    case TerrainLayout::CENTER_PILLAR:
        layoutCenterPillar(room);
        break;
    case TerrainLayout::FOUR_PILLARS:
        layoutFourPillars(room);
        break;
    case TerrainLayout::RANDOM_MESS:
        layoutRandomMess(room);
        break;
    case TerrainLayout::NONE:
    default:
        break;
    }
}

void TerrainLayoutHelper::addBoxCluster(Room* room, int centerTileX, int centerTileY, int width, int height)
{
    // 随机选择一种木箱材质（单堆内统一）
    int typeIndex = cocos2d::RandomHelper::random_int(0, 2);
    Box::BoxType type;
    switch (typeIndex)
    {
    case 0: type = Box::BoxType::NORMAL; break;
    case 1: type = Box::BoxType::LIGHT; break;
    case 2: type = Box::BoxType::DARK; break;
    default: type = Box::BoxType::NORMAL; break;
    }
    
    // 计算起始偏移（以中心为基准）
    int halfWidth = width / 2;
    int halfHeight = height / 2;
    int startX = -halfWidth;
    int startY = -halfHeight;
    int endX = startX + width - 1;
    int endY = startY + height - 1;
    
    // 放置指定大小的木箱堆
    for (int dy = startY; dy <= endY; dy++)
    {
        for (int dx = startX; dx <= endX; dx++)
        {
            room->addBoxAtTile(centerTileX + dx, centerTileY + dy, type);
        }
    }
}

void TerrainLayoutHelper::addPillarCluster(Room* room, int centerTileX, int centerTileY, int width, int height)
{
    // 随机选择一种石柱材质（单堆内统一）
    int typeIndex = cocos2d::RandomHelper::random_int(0, 2);
    Pillar::PillarType type;
    switch (typeIndex)
    {
    case 0: type = Pillar::PillarType::CLEAR; break;
    case 1: type = Pillar::PillarType::BROKEN; break;
    case 2: type = Pillar::PillarType::GLASSES; break;
    default: type = Pillar::PillarType::CLEAR; break;
    }
    
    // 根据width和height放置石柱堆
    int halfW = width / 2;
    int halfH = height / 2;
    int startX = centerTileX - halfW + (width % 2 == 0 ? 0 : 0);
    int startY = centerTileY - halfH + (height % 2 == 0 ? 0 : 0);
    
    for (int dy = 0; dy < height; dy++)
    {
        for (int dx = 0; dx < width; dx++)
        {
            room->addPillarAtTile(startX + dx, startY + dy, type);
        }
    }
}

void TerrainLayoutHelper::layoutFiveBoxes(Room* room)
{
    int tilesWidth = room->getTilesWidth();
    int tilesHeight = room->getTilesHeight();
    
    int centerX = tilesWidth / 2;
    int centerY = tilesHeight / 2;
    
    // 角落堆紧挨禁止放置区
    int leftX = 4;
    int rightX = tilesWidth - 5;
    int topY = tilesHeight - 5;
    int bottomY = 4;
    
    // 角落堆：3x3
    addBoxCluster(room, leftX, topY, 3, 3);      // 左上
    addBoxCluster(room, leftX, bottomY, 3, 3);   // 左下
    addBoxCluster(room, rightX, topY, 3, 3);     // 右上
    addBoxCluster(room, rightX, bottomY, 3, 3);  // 右下
    // 中心堆：4x4
    addBoxCluster(room, centerX, centerY, 4, 4);
}

void TerrainLayoutHelper::layoutNineBoxes(Room* room)
{
    int tilesWidth = room->getTilesWidth();
    int tilesHeight = room->getTilesHeight();
    
    int centerX = tilesWidth / 2;
    int centerY = tilesHeight / 2;
    int leftX = 4;
    int rightX = tilesWidth - 5;
    int topY = tilesHeight - 5;
    int bottomY = 4;
    
    // 角落堆：3x3
    addBoxCluster(room, leftX, topY, 3, 3);
    addBoxCluster(room, leftX, bottomY, 3, 3);
    addBoxCluster(room, rightX, topY, 3, 3);
    addBoxCluster(room, rightX, bottomY, 3, 3);
    
    // 中心堆：4x4
    addBoxCluster(room, centerX, centerY, 4, 4);
    
    // 边中堆：左右中3x4紧挨边缘，上下中4x3紧挨边缘
    addBoxCluster(room, leftX, centerY, 3, 4);
    addBoxCluster(room, rightX, centerY, 3, 4);
    addBoxCluster(room, centerX, topY, 4, 3);
    addBoxCluster(room, centerX, bottomY, 4, 3);
}

void TerrainLayoutHelper::layoutUpDownSpikes(Room* room)
{
    int tilesWidth = room->getTilesWidth();
    int tilesHeight = room->getTilesHeight();
    
    int innerLeft = 4;
    int innerRight = tilesWidth - 5;
    int innerTop = tilesHeight - 5;
    int innerBottom = 4;
    
    // 左右木箱（宽度1）
    for (int y = innerBottom; y <= innerTop; y++)
    {
        room->addBoxAtTile(innerLeft, y, Box::BoxType::NORMAL);
        room->addBoxAtTile(innerRight, y, Box::BoxType::NORMAL);
    }
    
    // 上下地刺（宽度1）
    for (int x = innerLeft + 1; x < innerRight; x++)
    {
        room->addSpikeAtTile(x, innerTop);
        room->addSpikeAtTile(x, innerBottom);
    }
}

void TerrainLayoutHelper::layoutLeftRightSpikes(Room* room)
{
    int tilesWidth = room->getTilesWidth();
    int tilesHeight = room->getTilesHeight();
    
    int innerLeft = 4;
    int innerRight = tilesWidth - 5;
    int innerTop = tilesHeight - 5;
    int innerBottom = 4;
    
    // 上下木箱（宽度1）
    for (int x = innerLeft; x <= innerRight; x++)
    {
        room->addBoxAtTile(x, innerTop, Box::BoxType::NORMAL);
        room->addBoxAtTile(x, innerBottom, Box::BoxType::NORMAL);
    }
    
    // 左右地刺（宽度1）
    for (int y = innerBottom + 1; y < innerTop; y++)
    {
        room->addSpikeAtTile(innerLeft, y);
        room->addSpikeAtTile(innerRight, y);
    }
}

void TerrainLayoutHelper::layoutAllSpikes(Room* room)
{
    int tilesWidth = room->getTilesWidth();
    int tilesHeight = room->getTilesHeight();
    
    int innerLeft = 4;
    int innerRight = tilesWidth - 5;
    int innerTop = tilesHeight - 5;
    int innerBottom = 4;
    
    // 上下地刺
    for (int x = innerLeft; x <= innerRight; x++)
    {
        room->addSpikeAtTile(x, innerTop);
        room->addSpikeAtTile(x, innerBottom);
    }
    
    // 左右地刺
    for (int y = innerBottom + 1; y < innerTop; y++)
    {
        room->addSpikeAtTile(innerLeft, y);
        room->addSpikeAtTile(innerRight, y);
    }
}

void TerrainLayoutHelper::layoutUpDownBoxes(Room* room)
{
    int tilesWidth = room->getTilesWidth();
    int tilesHeight = room->getTilesHeight();
    
    int wallY_top = tilesHeight - 4;
    int wallY_bottom = 3;
    
    for (int x = 3; x < tilesWidth - 3; x++)
    {
        room->addBoxAtTile(x, wallY_top, Box::BoxType::NORMAL);
        room->addBoxAtTile(x, wallY_bottom, Box::BoxType::NORMAL);
    }
}

void TerrainLayoutHelper::layoutLeftRightBoxes(Room* room)
{
    int tilesWidth = room->getTilesWidth();
    int tilesHeight = room->getTilesHeight();
    
    int wallX_left = 3;
    int wallX_right = tilesWidth - 4;
    
    for (int y = 3; y < tilesHeight - 3; y++)
    {
        room->addBoxAtTile(wallX_left, y, Box::BoxType::NORMAL);
        room->addBoxAtTile(wallX_right, y, Box::BoxType::NORMAL);
    }
}

void TerrainLayoutHelper::layoutCenterPillar(Room* room)
{
    int tilesWidth = room->getTilesWidth();
    int tilesHeight = room->getTilesHeight();
    
    int centerX = tilesWidth / 2;
    int centerY = tilesHeight / 2;
    
    addPillarCluster(room, centerX, centerY, 4, 4);
}

void TerrainLayoutHelper::layoutFourPillars(Room* room)
{
    int tilesWidth = room->getTilesWidth();
    int tilesHeight = room->getTilesHeight();
    
    int leftX = 3;
    int rightX = tilesWidth - 5;
    int topY = tilesHeight - 5;
    int bottomY = 3;
    
    addPillarCluster(room, leftX, topY, 2, 2);      // 左上
    addPillarCluster(room, leftX, bottomY, 2, 2);   // 左下
    addPillarCluster(room, rightX, topY, 2, 2);     // 右上
    addPillarCluster(room, rightX, bottomY, 2, 2);  // 右下
}

void TerrainLayoutHelper::layoutRandomMess(Room* room)
{
    int tilesWidth = room->getTilesWidth();
    int tilesHeight = room->getTilesHeight();
    
    // 放置15个石柱
    for (int i = 0; i < 15; i++)
    {
        int x = cocos2d::RandomHelper::random_int(3, tilesWidth - 4);
        int y = cocos2d::RandomHelper::random_int(3, tilesHeight - 4);
        
        int typeIndex = cocos2d::RandomHelper::random_int(0, 2);
        Pillar::PillarType type;
        switch (typeIndex)
        {
        case 0: type = Pillar::PillarType::CLEAR; break;
        case 1: type = Pillar::PillarType::BROKEN; break;
        case 2: type = Pillar::PillarType::GLASSES; break;
        default: type = Pillar::PillarType::CLEAR; break;
        }
        
        room->addPillarAtTile(x, y, type);
    }
    
    // 放置15个木箱
    for (int i = 0; i < 15; i++)
    {
        int x = cocos2d::RandomHelper::random_int(3, tilesWidth - 4);
        int y = cocos2d::RandomHelper::random_int(3, tilesHeight - 4);
        
        int typeIndex = cocos2d::RandomHelper::random_int(0, 2);
        Box::BoxType type;
        switch (typeIndex)
        {
        case 0: type = Box::BoxType::NORMAL; break;
        case 1: type = Box::BoxType::LIGHT; break;
        case 2: type = Box::BoxType::DARK; break;
        default: type = Box::BoxType::NORMAL; break;
        }
        
        room->addBoxAtTile(x, y, type);
    }
}
