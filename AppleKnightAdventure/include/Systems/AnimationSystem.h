#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include "raylib.h"

namespace Systems {

struct AnimationFrame {
    Rectangle src;
    float duration = 0.1f; // seconds
    Vector2 origin{0,0};
    // Optional metadata for trimmed/rotated frames (TexturePacker/Aseprite compat)
    bool rotated = false;            // frame stored rotated in atlas
    bool trimmed = false;            // whether the frame has been trimmed
    Vector2 spriteSourceSize{0,0};   // trimmed sprite offset (x,y) within the original source
    Vector2 originalSize{0,0};       // original source size before trimming (w,h stored in x,y)
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
    void AddClip(const std::shared_ptr<AnimationClip>& clip);
    bool HasClip(const std::string& name) const;

    // Playback control
    // Returns true if the clip exists and playback started
    bool Play(const std::string& name, float speed = 1.0f, bool reset = true);
    void Stop();
    void Pause();
    void Resume();
    bool IsPlaying() const;
    std::string CurrentClip() const;

    // Update per-frame
    void Update(float dt);

    // Seek playhead to seconds within the current clip (clamped)
    void Seek(float seconds);

    // Set current frame index directly (clamped) and reset internal timer
    void SetFrame(int index);

    // Get current playhead time into the current frame (seconds)
    float GetPlayheadTime() const;

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
    std::unordered_map<std::string, std::shared_ptr<AnimationClip>> m_clips;
    std::shared_ptr<AnimationClip> m_current = nullptr;
    std::string m_currentName;
    int m_frameIndex = 0;
    float m_timer = 0.0f;
    bool m_playing = false;
    float m_speed = 1.0f;
    bool m_flipX = false;
};

} // namespace Systems
