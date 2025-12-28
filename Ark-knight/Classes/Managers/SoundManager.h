#ifndef __SOUND_MANAGER_H__
#define __SOUND_MANAGER_H__

#include "cocos2d.h"
#include "audio/include/AudioEngine.h"  // cocos2d-x 4.0: AudioEngine 在 cocos2d 命名空间下
#include "Core/Constants.h"
#include "Core/GameMacros.h"
#include <string>
#include <unordered_map>

USING_NS_CC;

/**
 * 音频管理器（单例模式）
 * 统一管理背景音乐和音效播放
 */
class SoundManager {
public:
    /**
     * 获取单例实例
     */
    static SoundManager* getInstance();
    
    /**
     * 销毁单例
     */
    static void destroyInstance();
    
    // ==================== 背景音乐 ====================
    /**
     * 播放背景音乐
     * @param filePath 音频文件路径
     * @param loop 是否循环
     */
    void playBGM(const std::string& filePath, bool loop = true);
    
    /**
     * 停止背景音乐
     */
    void stopBGM();
    
    /**
     * 暂停背景音乐
     */
    void pauseBGM();
    
    /**
     * 继续背景音乐
     */
    void resumeBGM();
    
    /**
     * 设置背景音乐音量
     */
    void setBGMVolume(float volume);
    
    /**
     * 获取背景音乐音量
     */
    float getBGMVolume() const { return _bgmVolume; }
    
    // ==================== 音效 ====================
    /**     * 预加载音效
     */
    void preload(const std::string& filePath);

    /**     * 播放音效
     * @param filePath 音频文件路径
     * @param loop 是否循环
     * @return 音效ID
     */
    int playSFX(const std::string& filePath, bool loop = false);
    
    /**
     * 停止音效
     */
    void stopSFX(int audioID);
    
    /**
     * 停止所有音效
     */
    void stopAllSFX();
    
    /**
     * 设置音效音量
     */
    void setSFXVolume(float volume);
    
    /**
     * 获取音效音量
     */
    float getSFXVolume() const { return _sfxVolume; }
    
    // ==================== 全局控制 ====================
    /**
     * 设置静音
     */
    void setMute(bool mute);
    
    /**
     * 是否静音
     */
    bool isMuted() const { return _isMuted; }
    
    /**
     * 预加载音频
     */
    void preloadAudio(const std::string& filePath);
    
    /**
     * 卸载音频
     */
    void unloadAudio(const std::string& filePath);
    
private:
    SoundManager();
    ~SoundManager();
    
    // 禁止拷贝
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;
    
private:
    static SoundManager* _instance;
    
    int _bgmAudioID;              // 当前背景音乐ID（使用experimental::AudioEngine）
    float _bgmVolume;             // 背景音乐音量
    float _sfxVolume;             // 音效音量
    bool _isMuted;                // 是否静音
    
    std::unordered_map<int, std::string> _sfxMap;  // 音效ID映射表
};

#endif // __SOUND_MANAGER_H__
