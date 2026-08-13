#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

#include "raylib.h"
#include <string>
#include <unordered_map>
#include <vector>

class SoundManager {
protected:
    struct SoundEvent {
        std::vector<std::string> samples;
        float volume = 1.0f;
        float minPitch = 1.0f;
        float maxPitch = 1.0f;
        float cooldown = 0.0f;
        float cooldownRemaining = 0.0f;
        int lastSample = -1;
    };

    std::unordered_map<std::string, Sound> m_sounds;
    std::unordered_map<std::string, Music> m_music;
    std::unordered_map<std::string, SoundEvent> m_events;
    std::unordered_map<std::string, float> m_musicGains;
    std::string m_currentMusic;
    float m_sfxVolume;
    float m_musicVolume;
    bool m_audioInitialized;

    SoundManager();

public:
    SoundManager(const SoundManager&) = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    static SoundManager& GetInstance();

    bool InitAudio();
    void CloseAudio();
    bool IsAudioInitialized() const;

    bool LoadSound(const std::string& name, const std::string& filepath);
    bool LoadMusic(const std::string& name, const std::string& filepath);
    bool LoadManifest(const std::string& filepath);
    void Update(float dt);

    void PlaySound(const std::string& name);
    void StopSound(const std::string& name);
    void PauseSound(const std::string& name);
    void ResumeSound(const std::string& name);
    bool IsSoundPlaying(const std::string& name) const;

    void PlayMusic(const std::string& name);
    void StopMusic(const std::string& name);
    void PauseMusic(const std::string& name);
    void ResumeMusic(const std::string& name);
    void UpdateMusicStream(const std::string& name);
    
    void StopAllSounds();
    void StopAllMusic();
    bool IsMusicPlaying(const std::string& name) const;

    void SetSFXVolume(float volume);
    float GetSFXVolume() const;
    void SetMusicVolume(float volume);
    float GetMusicVolume() const;

    void UnloadAll();
};

#endif
