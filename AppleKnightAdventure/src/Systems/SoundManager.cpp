#include "Systems/SoundManager.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <utility>

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
        music.looping = true;
        m_music[name] = music;
        return true;
    }
    return false;
}

bool SoundManager::LoadManifest(const std::string& filepath) {
    if (!m_audioInitialized) return false;

    std::ifstream file(filepath);
    if (!file.is_open()) {
        TraceLog(LOG_WARNING, "AUDIO: Cannot open manifest: %s", filepath.c_str());
        return false;
    }

    try {
        nlohmann::json root;
        file >> root;

        UnloadAll();

        bool ok = true;
        const nlohmann::json& samples = root.at("samples");
        const nlohmann::json& musicConfig = root.at("music");
        const nlohmann::json& events = root.at("events");

        for (const auto& [name, pathValue] : samples.items()) {
            if (!pathValue.is_string() || !LoadSound(name, pathValue.get<std::string>())) {
                TraceLog(LOG_WARNING, "AUDIO: Failed to load sample '%s'", name.c_str());
                ok = false;
            }
        }

        for (const auto& [name, config] : musicConfig.items()) {
            const std::string path = config.value("file", std::string{});
            if (path.empty() || !LoadMusic(name, path)) {
                TraceLog(LOG_WARNING, "AUDIO: Failed to load music '%s'", name.c_str());
                ok = false;
                continue;
            }
            m_musicGains[name] = std::clamp(config.value("volume", 1.0f), 0.0f, 1.0f);
            ::SetMusicVolume(m_music[name], m_musicVolume * m_musicGains[name]);
        }

        for (const auto& [name, config] : events.items()) {
            SoundEvent event;
            event.samples = config.value("samples", std::vector<std::string>{});
            event.volume = std::clamp(config.value("volume", 1.0f), 0.0f, 1.0f);
            event.cooldown = std::max(0.0f, config.value("cooldown", 0.0f));
            if (config.contains("pitch") && config["pitch"].is_array() && config["pitch"].size() == 2) {
                event.minPitch = std::max(0.1f, config["pitch"][0].get<float>());
                event.maxPitch = std::max(event.minPitch, config["pitch"][1].get<float>());
            }

            event.samples.erase(
                std::remove_if(event.samples.begin(), event.samples.end(), [this, &name](const std::string& sample) {
                    if (m_sounds.find(sample) != m_sounds.end()) return false;
                    TraceLog(LOG_WARNING, "AUDIO: Event '%s' references missing sample '%s'", name.c_str(), sample.c_str());
                    return true;
                }),
                event.samples.end());

            if (!event.samples.empty()) m_events.emplace(name, std::move(event));
        }

        TraceLog(LOG_INFO, "AUDIO: Loaded %d samples, %d events and %d music tracks",
                 static_cast<int>(m_sounds.size()), static_cast<int>(m_events.size()),
                 static_cast<int>(m_music.size()));
        return ok && !m_events.empty();
    } catch (const std::exception& e) {
        TraceLog(LOG_WARNING, "AUDIO: Invalid manifest: %s", e.what());
        UnloadAll();
        return false;
    }
}

void SoundManager::Update(float dt) {
    if (!m_audioInitialized) return;
    for (auto& [name, event] : m_events) {
        (void)name;
        event.cooldownRemaining = std::max(0.0f, event.cooldownRemaining - dt);
    }
    for (auto& [name, music] : m_music) {
        (void)name;
        if (::IsMusicStreamPlaying(music)) ::UpdateMusicStream(music);
    }
}

void SoundManager::PlaySound(const std::string& name) {
    if (!m_audioInitialized) return;

    auto eventIt = m_events.find(name);
    if (eventIt != m_events.end()) {
        SoundEvent& event = eventIt->second;
        if (event.cooldownRemaining > 0.0f || event.samples.empty()) return;

        int index = 0;
        if (event.samples.size() > 1) {
            index = GetRandomValue(0, static_cast<int>(event.samples.size()) - 1);
            if (index == event.lastSample) index = (index + 1) % static_cast<int>(event.samples.size());
        }
        event.lastSample = index;

        auto sampleIt = m_sounds.find(event.samples[index]);
        if (sampleIt == m_sounds.end()) return;
        const float t = static_cast<float>(GetRandomValue(0, 10000)) / 10000.0f;
        const float pitch = event.minPitch + (event.maxPitch - event.minPitch) * t;
        ::SetSoundVolume(sampleIt->second, m_sfxVolume * event.volume);
        ::SetSoundPitch(sampleIt->second, pitch);
        ::PlaySound(sampleIt->second);
        event.cooldownRemaining = event.cooldown;
        return;
    }

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
        if (m_currentMusic == name && ::IsMusicStreamPlaying(it->second)) return;
        StopAllMusic();
        const auto gainIt = m_musicGains.find(name);
        const float gain = gainIt != m_musicGains.end() ? gainIt->second : 1.0f;
        ::SetMusicVolume(it->second, m_musicVolume * gain);
        ::PlayMusicStream(it->second);
        m_currentMusic = name;
    }
}

void SoundManager::StopMusic(const std::string& name) {
    auto it = m_music.find(name);
    if (it != m_music.end()) {
        ::StopMusicStream(it->second);
        if (m_currentMusic == name) m_currentMusic.clear();
    }
}

void SoundManager::StopAllSounds() {
    for (const auto& pair : m_sounds) {
        ::StopSound(pair.second);
    }
}

void SoundManager::StopAllMusic() {
    for (const auto& pair : m_music) {
        ::StopMusicStream(pair.second);
    }
    m_currentMusic.clear();
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
    m_sfxVolume = std::clamp(volume, 0.0f, 1.0f);
    for (auto& pair : m_sounds) {
        ::SetSoundVolume(pair.second, m_sfxVolume);
    }
}

float SoundManager::GetSFXVolume() const {
    return m_sfxVolume;
}

void SoundManager::SetMusicVolume(float volume) {
    m_musicVolume = std::clamp(volume, 0.0f, 1.0f);
    for (auto& pair : m_music) {
        const auto gainIt = m_musicGains.find(pair.first);
        const float gain = gainIt != m_musicGains.end() ? gainIt->second : 1.0f;
        ::SetMusicVolume(pair.second, m_musicVolume * gain);
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
    m_events.clear();
    for (auto& pair : m_music) {
        ::UnloadMusicStream(pair.second);
    }
    m_music.clear();
    m_musicGains.clear();
    m_currentMusic.clear();
}
