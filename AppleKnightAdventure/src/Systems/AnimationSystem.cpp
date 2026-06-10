#include "../../include/Systems/AnimationSystem.h"

namespace Systems {

Animator::Animator() = default;

void Animator::SetTexture(Texture2D* tex) { m_texture = tex; }
Texture2D* Animator::GetTexture() const { return m_texture; }

void Animator::AddClip(const AnimationClip& clip) {
    m_clips.emplace(clip.name, clip);
}

bool Animator::HasClip(const std::string& name) const {
    return m_clips.find(name) != m_clips.end();
}

void Animator::Play(const std::string& name, float speed, bool reset) {
    auto it = m_clips.find(name);
    if (it == m_clips.end()) return;
    if (m_currentName != name || reset) {
        m_current = &it->second;
        m_currentName = name;
        m_frameIndex = 0;
        m_timer = 0.0f;
    }
    m_playing = true;
    m_speed = speed;
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
bool Animator::IsPlaying() const { return m_playing && m_current; }
std::string Animator::CurrentClip() const { return m_currentName; }

void Animator::Update(float dt) {
    if (!m_playing || !m_current) return;
    if (m_current->frames.empty()) return;
    m_timer += dt * m_speed;
    const AnimationFrame& frame = m_current->frames[m_frameIndex];
    if (m_timer >= frame.duration) {
        m_timer -= frame.duration;
        int prev = m_frameIndex;
        m_frameIndex++;
        if (m_frameIndex >= (int)m_current->frames.size()) {
            if (m_current->loop) {
                m_frameIndex = 0;
            } else {
                m_frameIndex = (int)m_current->frames.size() - 1;
                m_playing = false;
                if (OnClipFinished) OnClipFinished(m_currentName);
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

} // namespace Systems
