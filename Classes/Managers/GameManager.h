#ifndef __GAME_MANAGER_H__
#define __GAME_MANAGER_H__

#include "cocos2d.h"
#include "Core/Constants.h"
#include "Core/GameMacros.h"

USING_NS_CC;

// 游戏管理器 - 管理全局游戏状态、关卡进度、分数等
class GameManager {
public:
    static GameManager* getInstance();
    static void destroyInstance();
    
    // 游戏状态管理
    int getCurrentLevel() const { return _currentLevel; }
    void setCurrentLevel(int level) { _currentLevel = level; }
    void nextLevel() { _currentLevel++; }
    
    int getScore() const { return _score; }
    void addScore(int points) { _score += points; }
    void resetScore() { _score = 0; }
    
    // 玩家数据管理
    int getSelectedCharacter() const { return _selectedCharacter; }
    void setSelectedCharacter(int character) { _selectedCharacter = character; }
    
    int getGold() const { return _gold; }
    void addGold(int amount) { _gold += amount; }
    bool spendGold(int amount);  // 返回是否有足够金币
    
    // 游戏重置
    void resetGame();
    
private:
    GameManager();
    ~GameManager();
    
    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;
    
private:
    static GameManager* _instance;
    
    int _currentLevel;        // 当前关卡
    int _score;               // 分数
    int _selectedCharacter;   // 选择的角色 (0=法师, 1=战士, 2=炮手)
    int _gold;                // 金币
};

#endif // __GAME_MANAGER_H__
