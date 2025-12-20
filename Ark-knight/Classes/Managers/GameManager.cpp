#include "GameManager.h"

GameManager* GameManager::_instance = nullptr;

GameManager* GameManager::getInstance()
{
    if (_instance == nullptr)
    {
        _instance = new (std::nothrow) GameManager();
        GAME_LOG("GameManager instance created");
    }
    return _instance;
}

void GameManager::destroyInstance()
{
    if (_instance != nullptr)
    {
        delete _instance;
        _instance = nullptr;
        GAME_LOG("GameManager instance destroyed");
    }
}

GameManager::GameManager()
    : _currentLevel(1)
    , _score(0)
    , _selectedCharacter(0)
    , _gold(0)
{
    GAME_LOG("GameManager initialized");
}

GameManager::~GameManager()
{
}

bool GameManager::spendGold(int amount)
{
    if (_gold >= amount)
    {
        _gold -= amount;
        GAME_LOG("Spent %d gold, remaining: %d", amount, _gold);
        return true;
    }
    
    GAME_LOG("Not enough gold! Need %d, have %d", amount, _gold);
    return false;
}

void GameManager::resetGame()
{
    _currentLevel = 1;
    _score = 0;
    _gold = 0;
    
    GAME_LOG("Game reset");
}
