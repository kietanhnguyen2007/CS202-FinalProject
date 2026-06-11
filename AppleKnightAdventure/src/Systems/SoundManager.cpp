#include "Systems/SoundManager.h"

SoundManager::SoundManager()
    : m_sfxVolume(1.0f)
    , m_musicVolume(1.0f)
    , m_audioInitialized(false)
{
}

SoundManager& SoundManager::GetInstance() {
    static SoundManager instance;
    return instance;
}

bool SoundManager::InitAudio() {
    if (!m_audioInitialized) {
        ::InitAudioDevice();
        m_audioInitialized = ::IsAudioDeviceReady();
    }
    return m_audioInitialized;
}

void SoundManager::CloseAudio() {
    UnloadAll();
    if (m_audioInitialized) {
        ::CloseAudioDevice();
        m_audioInitialized = false;
    }
}

bool SoundManager::IsAudioInitialized() const {
    return m_audioInitialized;
}

bool SoundManager::LoadSound(const std::string& name, const std::string& filepath) {
    if (!m_audioInitialized) return false;
    if (m_sounds.find(name) != m_sounds.end()) {
        ::UnloadSound(m_sounds[name]);
    }
    Sound sound = ::LoadSound(filepath.c_str());
    if (sound.frameCount > 0) {
        m_sounds[name] = sound;
        return true;
    }
    return false;
}

bool SoundManager::LoadMusic(const std::string& name, const std::string& filepath) {
    if (!m_audioInitialized) return false;
    if (m_music.find(name) != m_music.end()) {
        ::UnloadMusicStream(m_music[name]);
    }
    Music music = ::LoadMusicStream(filepath.c_str());
    if (music.frameCount > 0) {
        m_music[name] = music;
        return true;
    }
    return false;
}

void SoundManager::PlaySound(const std::string& name) {
    if (!m_audioInitialized) return;
    auto it = m_sounds.find(name);
    if (it != m_sounds.end()) {
        ::SetSoundVolume(it->second, m_sfxVolume);
        ::PlaySound(it->second);
    }
}

void SoundManager::StopSound(const std::string& name) {
    auto it = m_sounds.find(name);
    if (it != m_sounds.end()) {
        ::StopSound(it->second);
    }
}

void SoundManager::PauseSound(const std::string& name) {
    auto it = m_sounds.find(name);
    if (it != m_sounds.end()) {
        ::PauseSound(it->second);
    }
}

void SoundManager::ResumeSound(const std::string& name) {
    auto it = m_sounds.find(name);
    if (it != m_sounds.end()) {
        ::ResumeSound(it->second);
    }
}

bool SoundManager::IsSoundPlaying(const std::string& name) const {
    auto it = m_sounds.find(name);
    if (it != m_sounds.end()) {
        return ::IsSoundPlaying(it->second);
    }
    return false;
}

void SoundManager::PlayMusic(const std::string& name) {
    if (!m_audioInitialized) return;
    auto it = m_music.find(name);
    if (it != m_music.end()) {
        ::SetMusicVolume(it->second, m_musicVolume);
        ::PlayMusicStream(it->second);
    }
}

void SoundManager::StopMusic(const std::string& name) {
    auto it = m_music.find(name);
    if (it != m_music.end()) {
        ::StopMusicStream(it->second);
    }
}

void SoundManager::PauseMusic(const std::string& name) {
    auto it = m_music.find(name);
    if (it != m_music.end()) {
        ::PauseMusicStream(it->second);
    }
}

void SoundManager::ResumeMusic(const std::string& name) {
    auto it = m_music.find(name);
    if (it != m_music.end()) {
        ::ResumeMusicStream(it->second);
    }
}

void SoundManager::UpdateMusicStream(const std::string& name) {
    auto it = m_music.find(name);
    if (it != m_music.end()) {
        ::UpdateMusicStream(it->second);
    }
}

bool SoundManager::IsMusicPlaying(const std::string& name) const {
    auto it = m_music.find(name);
    if (it != m_music.end()) {
        return ::IsMusicStreamPlaying(it->second);
    }
    return false;
}

void SoundManager::SetSFXVolume(float volume) {
    m_sfxVolume = volume;
    for (auto& pair : m_sounds) {
        ::SetSoundVolume(pair.second, volume);
    }
}

float SoundManager::GetSFXVolume() const {
    return m_sfxVolume;
}

void SoundManager::SetMusicVolume(float volume) {
    m_musicVolume = volume;
    for (auto& pair : m_music) {
        ::SetMusicVolume(pair.second, volume);
    }
}

float SoundManager::GetMusicVolume() const {
    return m_musicVolume;
}

void SoundManager::UnloadAll() {
    for (auto& pair : m_sounds) {
        ::UnloadSound(pair.second);
    }
    m_sounds.clear();
    for (auto& pair : m_music) {
        ::UnloadMusicStream(pair.second);
    }
    m_music.clear();
}
