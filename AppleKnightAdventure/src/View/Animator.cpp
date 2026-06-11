#include "View/Animator.h"
#include <cmath>
#include <algorithm>

// Small epsilon to make playhead -> frame mapping robust against
// floating point rounding when the playhead is extremely close to
// a frame boundary (in seconds).
static constexpr float PLAYHEAD_EPS = 1e-5f;

namespace View::Animations {

Animator::Animator() = default;

void Animator::SetTexture(Texture2D* tex) { m_texture = tex; }
Texture2D* Animator::GetTexture() const { return m_texture; }

void Animator::AddClip(const std::shared_ptr<AnimationClip>& clip) {
    if (!clip) return;
    // Cache total duration to avoid O(n) recomputation per Update/Seek
    if (clip->totalDuration <= 0.0f) {
        float total = 0.0f;
        for (const auto& f : clip->frames) {
            float d = f.duration;
            if (!(d > 0.0f)) d = 0.001f;
            total += d;
        }
        clip->totalDuration = total;
    }
    m_clips.emplace(clip->name, clip);
}

bool Animator::HasClip(const std::string& name) const {
    return m_clips.find(name) != m_clips.end();
}

bool Animator::Play(const std::string& name, float speed, bool reset) {
    auto it = m_clips.find(name);
    if (it == m_clips.end()) return false;
    auto clipPtr = it->second;
    if (m_currentName != name || reset) {
        m_current = clipPtr;
        m_currentName = name;
        m_frameIndex = 0;
        m_timer = 0.0f;
        // initialize playhead and playback helpers
        // If reset requested, position playhead at start or end depending on playback mode
        float clipLength = m_current->totalDuration;
        if (clipLength <= 0.0f) clipLength = m_current->frames.size() * 0.001f;
        if (m_playbackMode == PlaybackMode::Reverse) {
            m_playhead = reset ? clipLength : 0.0f;
            m_playDirection = -1;
        } else {
            m_playhead = 0.0f;
            m_playDirection = 1;
        }
        // set starting frame/time according to playhead
        Seek(m_playhead);
    }
    m_playing = true;
    m_speed = speed;
    return true;
}

void Animator::Stop() {
    m_playing = false;
    m_current = nullptr;
    m_currentName.clear();
    m_frameIndex = 0;
    m_timer = 0.0f;
}

void Animator::Pause() { m_playing = false; }
void Animator::Resume() { if (m_current) m_playing = true; }
bool Animator::IsPlaying() const { return m_playing && m_current != nullptr; }
std::string Animator::CurrentClip() const { return m_currentName; }

void Animator::Update(float dt) {
    if (!m_playing || !m_current) return;
    if (m_current->frames.empty()) return;

    // Advance absolute playhead based on playback direction
    int dir = m_playDirection;
    if (m_playbackMode == PlaybackMode::Reverse) dir = -1;
    // In PingPong mode we use m_playDirection to track current movement
    if (m_playbackMode == PlaybackMode::PingPong) dir = m_playDirection;

    m_playhead += dt * m_speed * (float)dir;

    // compute clip length
    float clipLength = m_current->totalDuration;
    if (clipLength <= 0.0f) clipLength = m_current->frames.size() * 0.001f;
    if (clipLength <= 0.0f) return;

    // Handle looping, clamping and ping-pong
    if (m_playbackMode == PlaybackMode::Normal || m_playbackMode == PlaybackMode::Reverse) {
        if (m_current->loop) {
            // wrap into [0, clipLength) using fmod to avoid repeated-subtraction
            m_playhead = std::fmod(m_playhead, clipLength);
            if (m_playhead < 0.0f) m_playhead += clipLength;
        } else {
            // clamp to [0, clipLength]
            if (m_playhead < 0.0f) {
                m_playhead = 0.0f;
                m_playing = false;
                if (OnClipFinished) OnClipFinished(m_currentName);
            }
            if (m_playhead > clipLength) {
                m_playhead = clipLength;
                m_playing = false;
                if (OnClipFinished) OnClipFinished(m_currentName);
            }
        }
    } else if (m_playbackMode == PlaybackMode::PingPong) {
        // For ping-pong, reflect around bounds using period normalization
        float period = clipLength * 2.0f;
        m_playhead = std::fmod(m_playhead, period);
        if (m_playhead < 0.0f) m_playhead += period;
        if (m_playhead >= clipLength) {
            // mirrored phase
            m_playDirection = -1;
            m_playhead = period - m_playhead; // reflect to [0, clipLength]
        } else {
            m_playDirection = 1;
        }
    }

    // Recalculate frame index and timer from playhead
    float acc = 0.0f;
    int newIndex = 0;
    float newTimer = 0.0f;
    for (size_t i = 0; i < m_current->frames.size(); ++i) {
        float d = m_current->frames[i].duration;
        if (!(d > 0.0f)) d = 0.001f;
        // Use a small epsilon to ensure values extremely close to the upper
        // boundary are classified consistently with expectations (avoid
        // floating-point rounding pushing playhead to previous frame).
        if (m_playhead < acc + d - PLAYHEAD_EPS) {
            newIndex = (int)i;
            newTimer = m_playhead - acc;
            break;
        }
        acc += d;
    }

    // If no frame matched (e.g. playhead at exact clipLength for non-looping
    // clips), map to the last frame as a fallback (consistent with Seek()).
    if (newIndex == 0 && m_playhead + PLAYHEAD_EPS >= clipLength) {
        newIndex = (int)m_current->frames.size() - 1;
        newTimer = 0.0f;
    }

    int prevIndex = m_frameIndex;
    m_frameIndex = newIndex;
    m_timer = newTimer;

    if (OnFrameChanged && prevIndex != m_frameIndex) OnFrameChanged(m_currentName, m_frameIndex);
}

Rectangle Animator::GetCurrentSrcRect() const {
    if (!m_current || m_current->frames.empty()) return {0,0,0,0};
    return m_current->frames[m_frameIndex].src;
}

Vector2 Animator::GetCurrentOrigin() const {
    if (!m_current || m_current->frames.empty()) return {0,0};
    return m_current->frames[m_frameIndex].origin;
}

bool Animator::GetFlipX() const { return m_flipX; }
void Animator::SetFlipX(bool flip) { m_flipX = flip; }
bool Animator::HasTexture() const { return m_texture != nullptr && m_texture->id != 0; }

int Animator::GetCurrentFrameIndex() const { return m_frameIndex; }

int Animator::GetTotalFrames() const {
    return m_current ? (int)m_current->frames.size() : 0;
}

void Animator::Seek(float seconds) {
    if (!m_current || m_current->frames.empty()) return;
    // Compute clip length
    float clipLength = m_current->totalDuration;
    if (clipLength <= 0.0f) clipLength = m_current->frames.size() * 0.001f;
    if (clipLength <= 0.0f) return;

    float target = seconds;
    if (target < 0.0f) target = 0.0f;
    if (m_current->loop) {
        // wrap using fmod
        target = std::fmod(target, clipLength);
        if (target < 0.0f) target += clipLength;
    } else {
        if (target < 0.0f) target = 0.0f;
        if (target > clipLength) target = clipLength;
    }
    m_playhead = target;
    // recompute frame index and timer
    float acc = 0.0f;
    for (size_t i = 0; i < m_current->frames.size(); ++i) {
        float d = m_current->frames[i].duration;
        if (!(d > 0.0f)) d = 0.001f;
        if (m_playhead < acc + d - PLAYHEAD_EPS) {
            m_frameIndex = (int)i;
            m_timer = m_playhead - acc;
            return;
        }
        acc += d;
    }
    // fallback to last frame
    m_frameIndex = (int)m_current->frames.size() - 1;
    m_timer = 0.0f;
}

void Animator::SetFrame(int index) {
    if (!m_current || m_current->frames.empty()) return;
    if (index < 0) index = 0;
    if (index >= (int)m_current->frames.size()) index = (int)m_current->frames.size() - 1;
    // compute playhead corresponding to start of this frame
    float acc = 0.0f;
    for (int i = 0; i < index; ++i) {
        float d = m_current->frames[i].duration;
        if (!(d > 0.0f)) d = 0.001f;
        acc += d;
    }
    m_playhead = acc;
    m_frameIndex = index;
    m_timer = 0.0f;
}

float Animator::GetPlayheadTime() const {
    return m_playhead;
}

void Animator::SetPlaybackMode(PlaybackMode mode) {
    m_playbackMode = mode;
    if (mode == PlaybackMode::Reverse) m_playDirection = -1; else m_playDirection = 1;
}

Animator::PlaybackMode Animator::GetPlaybackMode() const {
    return m_playbackMode;
}

} // namespace View::Animations
