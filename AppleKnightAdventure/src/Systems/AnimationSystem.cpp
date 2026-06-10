#include "../../include/Systems/AnimationSystem.h"

namespace Systems {

Animator::Animator() = default;

void Animator::SetTexture(Texture2D* tex) { m_texture = tex; }
Texture2D* Animator::GetTexture() const { return m_texture; }

void Animator::AddClip(const std::shared_ptr<AnimationClip>& clip) {
    if (!clip) return;
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
    m_timer += dt * m_speed;

    // Safety cap to avoid infinite loops on malformed durations
    const int SAFE_MAX = std::max((int)m_current->frames.size() * 4, 1000);
    int advances = 0;

    while (advances++ < SAFE_MAX) {
        if (m_current->frames.empty()) break;
        const AnimationFrame& frame = m_current->frames[m_frameIndex];
        float dur = frame.duration;
        if (!(dur > 0.0f)) {
            // clamp invalid durations to a tiny epsilon to avoid infinite loops
            dur = 0.001f;
        }
        if (m_timer < dur) break;
        m_timer -= dur;
        int prev = m_frameIndex;
        m_frameIndex++;
        if (m_frameIndex >= (int)m_current->frames.size()) {
            if (m_current->loop) {
                m_frameIndex = 0;
            } else {
                m_frameIndex = (int)m_current->frames.size() - 1;
                m_playing = false;
                if (OnClipFinished) OnClipFinished(m_currentName);
                break;
            }
        }
        if (OnFrameChanged && prev != m_frameIndex) OnFrameChanged(m_currentName, m_frameIndex);
    }
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
    float t = seconds;
    if (t < 0.0f) t = 0.0f;
    m_frameIndex = 0;
    m_timer = 0.0f;
    for (size_t i = 0; i < m_current->frames.size(); ++i) {
        float dur = m_current->frames[i].duration;
        if (!(dur > 0.0f)) dur = 0.001f;
        if (t < dur) {
            m_frameIndex = (int)i;
            m_timer = t;
            return;
        }
        t -= dur;
    }
    // If seek past end, set to last frame and stop
    m_frameIndex = (int)m_current->frames.size() - 1;
    m_timer = 0.0f;
    m_playing = false;
}

void Animator::SetFrame(int index) {
    if (!m_current || m_current->frames.empty()) return;
    if (index < 0) index = 0;
    if (index >= (int)m_current->frames.size()) index = (int)m_current->frames.size() - 1;
    m_frameIndex = index;
    m_timer = 0.0f;
}

float Animator::GetPlayheadTime() const {
    if (!m_current || m_current->frames.empty()) return 0.0f;
    float t = m_timer;
    for (int i = 0; i < m_frameIndex; ++i) {
        float dur = m_current->frames[i].duration;
        if (!(dur > 0.0f)) dur = 0.001f;
        t += dur;
    }
    return t;
}

} // namespace Systems
