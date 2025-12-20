#ifndef __GAME_MANAGER_H__
#define __GAME_MANAGER_H__

#include "cocos2d.h"
#include "Core/Constants.h"
#include "Core/GameMacros.h"

USING_NS_CC;

/**
 * 游戏管理器（单例模式）
 * 管理全局游戏状态、关卡进度、分数等
 */
class GameManager {
public:
    /**
     * 获取单例实例
     */
    static GameManager* getInstance();
    
    /**
     * 销毁单例
     */
    static void destroyInstance();
    
    // ==================== 游戏状态 ====================
    /**
     * 获取当前关卡
     */
    int getCurrentLevel() const { return _currentLevel; }
    
    /**
     * 设置当前关卡
     */
    void setCurrentLevel(int level) { _currentLevel = level; }
    
    /**
     * 下一关
     */
    void nextLevel() { _currentLevel++; }
    
    /**
     * 获取分数
     */
    int getScore() const { return _score; }
    
    /**
     * 增加分数
     */
    void addScore(int points) { _score += points; }
    
    /**
     * 重置分数
     */
    void resetScore() { _score = 0; }
    
    // ==================== 玩家数据 ====================
    /**
     * 获取选择的角色类型
     */
    int getSelectedCharacter() const { return _selectedCharacter; }
    
    /**
     * 设置选择的角色
     */
    void setSelectedCharacter(int character) { _selectedCharacter = character; }
    
    /**
     * 获取金币数量
     */
    int getGold() const { return _gold; }
    
    /**
     * 增加金币
     */
    void addGold(int amount) { _gold += amount; }
    
    /**
     * 消耗金币
     */
    bool spendGold(int amount);
    
    // ==================== 游戏重置 ====================
    /**
     * 重置游戏（新游戏）
     */
    void resetGame();
    
private:
    GameManager();
    ~GameManager();
    
    // 禁止拷贝
    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;
    
private:
    static GameManager* _instance;
    
    int _currentLevel;        // 当前关卡
    int _score;               // 分数
    int _selectedCharacter;   // 选择的角色 (0=法师, 1=战士, 2=炼金术士)
    int _gold;                // 金币
};

#endif // __GAME_MANAGER_H__
