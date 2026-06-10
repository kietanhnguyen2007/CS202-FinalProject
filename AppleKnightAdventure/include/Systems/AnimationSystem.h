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

    // Pixel offset from top-left of source image to sprite's intended origin (trim offset, anchor, or cropping).
    Vector2 origin{0,0};

    // Optional metadata for trimmed/rotated frames (TexturePacker/Aseprite compat)
    bool rotated = false;          // frame stored rotated in atlas
    bool trimmed = false;          // whether the frame has been trimmed
    Vector2 spriteSourceSize{0,0}; // top-left offset of trimmed content within original sprite
    Vector2 originalSize{0,0};     // full original sprite size (width,height) before trimming
    // Optional name of the frame from the atlas (if provided). Useful to correlate
    // metadata and for debugging. May be empty for manually-constructed frames.
    std::string name;
};

struct AnimationClip {
    std::string name;
    std::vector<AnimationFrame> frames;
    bool loop = true;
    float totalDuration = 0.0f; // cached sum of all frame durations
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

    enum class PlaybackMode {
        Normal = 0,
        Reverse,
        PingPong
    };

    void SetPlaybackMode(PlaybackMode mode);
    PlaybackMode GetPlaybackMode() const;

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
    // Playback control
    PlaybackMode m_playbackMode = PlaybackMode::Normal;
    int m_playDirection = 1; // +1 forward, -1 backward (used for Reverse and PingPong)
    float m_playhead = 0.0f; // absolute time into the clip (seconds)
};

} // namespace Systems
