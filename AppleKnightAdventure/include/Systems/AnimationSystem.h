#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include "raylib.h"

namespace Systems {

struct AnimationFrame {
    Rectangle src;
    float duration = 0.1f; // seconds
    Vector2 origin{0,0};
};

struct AnimationClip {
    std::string name;
    std::vector<AnimationFrame> frames;
    bool loop = true;
};

class Animator {
public:
    Animator();

    // Resource binding
    void SetTexture(Texture2D* tex);
    Texture2D* GetTexture() const;

    // Clip management
    void AddClip(const AnimationClip& clip);
    bool HasClip(const std::string& name) const;

    // Playback control
    void Play(const std::string& name, float speed = 1.0f, bool reset = true);
    void Stop();
    void Pause();
    void Resume();
    bool IsPlaying() const;
    std::string CurrentClip() const;

    // Update per-frame
    void Update(float dt);

    // Query for rendering
    Rectangle GetCurrentSrcRect() const;
    Vector2 GetCurrentOrigin() const;
    bool GetFlipX() const;
    void SetFlipX(bool flip);
    bool HasTexture() const;

    // Debug / introspection
    int GetCurrentFrameIndex() const;
    int GetTotalFrames() const;

    // Events
    std::function<void(const std::string&, int)> OnFrameChanged;
    std::function<void(const std::string&)> OnClipFinished;

private:
    Texture2D* m_texture = nullptr;
    std::unordered_map<std::string, AnimationClip> m_clips;
    AnimationClip* m_current = nullptr;
    std::string m_currentName;
    int m_frameIndex = 0;
    float m_timer = 0.0f;
    bool m_playing = false;
    float m_speed = 1.0f;
    bool m_flipX = false;
};

} // namespace Systems
