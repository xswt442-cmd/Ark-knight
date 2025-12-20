#include "SoundManager.h"

// cocos2d-x 4.0 中 AudioEngine 直接在 cocos2d 命名空间下
using cocos2d::AudioEngine;

SoundManager* SoundManager::_instance = nullptr;

SoundManager* SoundManager::getInstance()
{
    if (_instance == nullptr)
    {
        _instance = new (std::nothrow) SoundManager();
        GAME_LOG("SoundManager instance created");
    }
    return _instance;
}

void SoundManager::destroyInstance()
{
    if (_instance != nullptr)
    {
        delete _instance;
        _instance = nullptr;
        GAME_LOG("SoundManager instance destroyed");
    }
}

SoundManager::SoundManager()
    : _bgmAudioID(AudioEngine::INVALID_AUDIO_ID)
    , _bgmVolume(0.5f)
    , _sfxVolume(0.8f)
    , _isMuted(false)
{
    GAME_LOG("SoundManager initialized");
}

SoundManager::~SoundManager()
{
    stopBGM();
    stopAllSFX();
}

void SoundManager::playBGM(const std::string& filePath, bool loop)
{
    if (_isMuted)
    {
        return;
    }
    
    // 停止当前BGM
    stopBGM();
    
    // 播放新BGM
    _bgmAudioID = AudioEngine::play2d(filePath, loop, _bgmVolume);
    
    if (_bgmAudioID != AudioEngine::INVALID_AUDIO_ID)
    {
        GAME_LOG("Playing BGM: %s", filePath.c_str());
    }
    else
    {
        GAME_LOG_ERROR("Failed to play BGM: %s", filePath.c_str());
    }
}

void SoundManager::stopBGM()
{
    if (_bgmAudioID != AudioEngine::INVALID_AUDIO_ID)
    {
        AudioEngine::stop(_bgmAudioID);
        _bgmAudioID = AudioEngine::INVALID_AUDIO_ID;
        GAME_LOG("BGM stopped");
    }
}

void SoundManager::pauseBGM()
{
    if (_bgmAudioID != AudioEngine::INVALID_AUDIO_ID)
    {
        AudioEngine::pause(_bgmAudioID);
        GAME_LOG("BGM paused");
    }
}

void SoundManager::resumeBGM()
{
    if (_bgmAudioID != AudioEngine::INVALID_AUDIO_ID)
    {
        AudioEngine::resume(_bgmAudioID);
        GAME_LOG("BGM resumed");
    }
}

void SoundManager::setBGMVolume(float volume)
{
    _bgmVolume = std::max(0.0f, std::min(1.0f, volume));
    
    if (_bgmAudioID != AudioEngine::INVALID_AUDIO_ID)
    {
        AudioEngine::setVolume(_bgmAudioID, _bgmVolume);
    }
    
    GAME_LOG("BGM volume set to %.2f", _bgmVolume);
}

int SoundManager::playSFX(const std::string& filePath, bool loop)
{
    if (_isMuted)
    {
        return AudioEngine::INVALID_AUDIO_ID;
    }
    
    int audioID = AudioEngine::play2d(filePath, loop, _sfxVolume);
    
    if (audioID != AudioEngine::INVALID_AUDIO_ID)
    {
        _sfxMap[audioID] = filePath;
        GAME_LOG("Playing SFX: %s (ID: %d)", filePath.c_str(), audioID);
    }
    else
    {
        GAME_LOG_ERROR("Failed to play SFX: %s", filePath.c_str());
    }
    
    return audioID;
}

void SoundManager::stopSFX(int audioID)
{
    if (audioID != AudioEngine::INVALID_AUDIO_ID)
    {
        AudioEngine::stop(audioID);
        _sfxMap.erase(audioID);
        GAME_LOG("SFX stopped (ID: %d)", audioID);
    }
}

void SoundManager::stopAllSFX()
{
    for (auto& pair : _sfxMap)
    {
        AudioEngine::stop(pair.first);
    }
    _sfxMap.clear();
    GAME_LOG("All SFX stopped");
}

void SoundManager::setSFXVolume(float volume)
{
    _sfxVolume = std::max(0.0f, std::min(1.0f, volume));
    
    // 更新所有正在播放的音效音量
    for (auto& pair : _sfxMap)
    {
        AudioEngine::setVolume(pair.first, _sfxVolume);
    }
    
    GAME_LOG("SFX volume set to %.2f", _sfxVolume);
}

void SoundManager::setMute(bool mute)
{
    _isMuted = mute;
    
    if (_isMuted)
    {
        pauseBGM();
        stopAllSFX();
        GAME_LOG("Sound muted");
    }
    else
    {
        resumeBGM();
        GAME_LOG("Sound unmuted");
    }
}

void SoundManager::preloadAudio(const std::string& filePath)
{
    AudioEngine::preload(filePath, [filePath](bool success) {
        if (success)
        {
            GAME_LOG("Audio preloaded: %s", filePath.c_str());
        }
        else
        {
            GAME_LOG_ERROR("Failed to preload audio: %s", filePath.c_str());
        }
    });
}

void SoundManager::unloadAudio(const std::string& filePath)
{
    AudioEngine::uncache(filePath);
    GAME_LOG("Audio unloaded: %s", filePath.c_str());
}
