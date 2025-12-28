#ifndef __SOUND_MANAGER_H__
#define __SOUND_MANAGER_H__

#include "cocos2d.h"
#include "audio/include/AudioEngine.h"
#include "Core/Constants.h"
#include "Core/GameMacros.h"
#include <string>
#include <unordered_map>

USING_NS_CC;

// 音频管理器 - 单例模式
class SoundManager {
public:
    static SoundManager* getInstance();
    static void destroyInstance();
    
    // 背景音乐控制
    void playBGM(const std::string& filePath, bool loop = true);
    void stopBGM();
    void pauseBGM();
    void resumeBGM();
    void setBGMVolume(float volume);
    float getBGMVolume() const { return _bgmVolume; }
    
    // 音效控制
    void preload(const std::string& filePath);
    int playSFX(const std::string& filePath, bool loop = false);
    void stopSFX(int audioID);
    void stopAllSFX();
    void setSFXVolume(float volume);
    float getSFXVolume() const { return _sfxVolume; }
    
    // 全局音频控制
    void setMute(bool mute);
    bool isMuted() const { return _isMuted; }
    void preloadAudio(const std::string& filePath);
    void unloadAudio(const std::string& filePath);
    
private:
    SoundManager();
    ~SoundManager();
    
    // 禁止拷贝
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;
    
private:
    static SoundManager* _instance;
    
    int _bgmAudioID;       // 当前背景音乐ID
    float _bgmVolume;      // 背景音乐音量
    float _sfxVolume;      // 音效音量
    bool _isMuted;         // 是否静音
    
    std::unordered_map<int, std::string> _sfxMap;  // 音效ID映射表
};

#endif
