#include "SoundManager.h"

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
        // 1. 先清除所有回调，避免回调访问已释放的内存
        if (_instance->_bgmAudioID != AudioEngine::INVALID_AUDIO_ID)
        {
            // 清除回调后再停止
            AudioEngine::setFinishCallback(_instance->_bgmAudioID, nullptr);
            AudioEngine::stop(_instance->_bgmAudioID);
            _instance->_bgmAudioID = AudioEngine::INVALID_AUDIO_ID;
        }
        
        // 停止 SFX 并清除回调
        for (auto &kv : _instance->_sfxMap)
        {
            AudioEngine::setFinishCallback(kv.first, nullptr);
            AudioEngine::stop(kv.first);
        }
        _instance->_sfxMap.clear();

        // PS: 不要在这里调用 AudioEngine::end() —— AppDelegate 负责在程序退出时调用它
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
    // 析构时清理映射并确保不再保留任何 ID
    _sfxMap.clear();
}

void SoundManager::playBGM(const std::string& filePath, bool loop)
{
    if (_isMuted)
    {
        return;
    }

    // 停止当前BGM（如果有）
    if (_bgmAudioID != AudioEngine::INVALID_AUDIO_ID)
    {
        AudioEngine::stop(_bgmAudioID);
        _bgmAudioID = AudioEngine::INVALID_AUDIO_ID;
    }

    // 播放新BGM
    _bgmAudioID = AudioEngine::play2d(filePath, loop, _bgmVolume);

    if (_bgmAudioID != AudioEngine::INVALID_AUDIO_ID)
    {
        // 当 BGM 结束或被停止时，重置 ID
        AudioEngine::setFinishCallback(_bgmAudioID, [this](int id, const std::string& path){
            if (id == _bgmAudioID) {
                _bgmAudioID = AudioEngine::INVALID_AUDIO_ID;
            }
        });
        GAME_LOG("Playing BGM: %s (ID: %d)", filePath.c_str(), _bgmAudioID);
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

    // 只要当前有 BGM 播放，则设置其音量；否则仅保存 _bgmVolume，以便下次 playBGM 时生效
    if (_bgmAudioID != AudioEngine::INVALID_AUDIO_ID)
    {
        AudioEngine::setVolume(_bgmAudioID, _bgmVolume);
    }

    GAME_LOG("BGM volume set to %.2f", _bgmVolume);
}

void SoundManager::preload(const std::string& filePath)
{
    if (!FileUtils::getInstance()->isFileExist(filePath))
    {
        GAME_LOG_ERROR("Preload failed: file not found %s", filePath.c_str());
        return;
    }

    AudioEngine::preload(filePath, [filePath](bool isSuccess){
        if (isSuccess)
        {
            GAME_LOG("Preload success: %s", filePath.c_str());
        }
        else
        {
            GAME_LOG_ERROR("Preload failed: %s", filePath.c_str());
        }
    });
    GAME_LOG("Preloading: %s", filePath.c_str());
}

int SoundManager::playSFX(const std::string& filePath, bool loop)
{
    if (_isMuted)
    {
        return AudioEngine::INVALID_AUDIO_ID;
    }

    // 检查文件是否存在
    if (!FileUtils::getInstance()->isFileExist(filePath))
    {
        GAME_LOG_ERROR("SFX file not found: %s", filePath.c_str());
        return AudioEngine::INVALID_AUDIO_ID;
    }

    // 如果没有预加载，先预加载一次（同步等待）
    AudioEngine::preload(filePath);

    int audioID = AudioEngine::play2d(filePath, loop, _sfxVolume);

    if (audioID != AudioEngine::INVALID_AUDIO_ID)
    {
        _sfxMap[audioID] = filePath;

        // 自动清理回调，避免 map 膨胀
        AudioEngine::setFinishCallback(audioID, [this](int id, const std::string& path){
            auto it = _sfxMap.find(id);
            if (it != _sfxMap.end()) {
                _sfxMap.erase(it);
            }
        });

        GAME_LOG("Playing SFX: %s (ID: %d, Vol: %.2f)", filePath.c_str(), audioID, _sfxVolume);
    }
    else
    {
        GAME_LOG_ERROR("Failed to play SFX: %s (AudioEngine returned INVALID_ID)", filePath.c_str());
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
    // 复制 keys 后停止，避免在遍历时修改 map
    std::vector<int> ids;
    ids.reserve(_sfxMap.size());
    for (auto &kv : _sfxMap) ids.push_back(kv.first);
    for (int id : ids) AudioEngine::stop(id);
    _sfxMap.clear();
    GAME_LOG("All SFX stopped");
}

void SoundManager::setSFXVolume(float volume)
{
    _sfxVolume = std::max(0.0f, std::min(1.0f, volume));

    // 更新所有正在播放的音效音量（只会包含正在被跟踪的 SFX IDs）
    for (auto &kv : _sfxMap)
    {
        AudioEngine::setVolume(kv.first, _sfxVolume);
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
