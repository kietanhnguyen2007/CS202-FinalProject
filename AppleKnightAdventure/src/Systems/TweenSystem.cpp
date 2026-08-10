#include "Systems/TweenSystem.h"
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

static float EaseLinear(float t) {
    return t;
}

static float EaseInOutQuad(float t) {
    return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

static float EaseOutQuad(float t) {
    return 1.0f - (1.0f - t) * (1.0f - t);
}

static float EaseOutCubic(float t) {
    return 1.0f - std::pow(1.0f - t, 3.0f);
}

static float EaseOutBack(float t) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    return 1.0f + c3 * std::pow(t - 1.0f, 3.0f) + c1 * std::pow(t - 1.0f, 2.0f);
}

static float EaseOutBounce(float t) {
    const float n1 = 7.5625f;
    const float d1 = 2.75f;

    if (t < 1.0f / d1) {
        return n1 * t * t;
    } else if (t < 2.0f / d1) {
        t -= 1.5f / d1;
        return n1 * t * t + 0.75f;
    } else if (t < 2.5f / d1) {
        t -= 2.25f / d1;
        return n1 * t * t + 0.9375f;
    } else {
        t -= 2.625f / d1;
        return n1 * t * t + 0.984375f;
    }
}

static float EaseOutElastic(float t) {
    const float c4 = (2.0f * M_PI) / 3.0f;

    if (t == 0.0f) return 0.0f;
    if (t == 1.0f) return 1.0f;

    return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
}

static float EaseInOutSine(float t) {
    return -(std::cos(M_PI * t) - 1.0f) / 2.0f;
}

TweenSystem& TweenSystem::GetInstance() {
    static TweenSystem instance;
    return instance;
}

float TweenSystem::Apply(EasingType e, float t) {
    switch (e) {
        case EasingType::Linear: return EaseLinear(t);
        case EasingType::InOutQuad: return EaseInOutQuad(t);
        case EasingType::OutQuad: return EaseOutQuad(t);
        case EasingType::OutCubic: return EaseOutCubic(t);
        case EasingType::OutBack: return EaseOutBack(t);
        case EasingType::OutBounce: return EaseOutBounce(t);
        case EasingType::OutElastic: return EaseOutElastic(t);
        case EasingType::InOutSine: return EaseInOutSine(t);
        default: return t;
    }
}

Tween& TweenSystem::Add(float* target, float from, float to, float duration, EasingType easing, bool loop, bool pingPong) {
    Tween* freeSlot = nullptr;
    for (auto& tween : m_tweens) {
        if (!tween.active) {
            freeSlot = &tween;
            break;
        }
    }

    if (!freeSlot) {
        if (m_tweens.size() < 256) {
            m_tweens.push_back(Tween());
            freeSlot = &m_tweens.back();
        } else {
            freeSlot = &m_tweens[0]; // overwrite oldest/first
        }
    }

    freeSlot->target = target;
    freeSlot->from = from;
    freeSlot->to = to;
    freeSlot->duration = duration;
    freeSlot->elapsed = 0.0f;
    freeSlot->easing = easing;
    freeSlot->loop = loop;
    freeSlot->pingPong = pingPong;
    freeSlot->active = true;
    freeSlot->pingPongDir = true;
    freeSlot->onComplete = nullptr;

    if (target) {
        *target = from;
    }

    return *freeSlot;
}

void TweenSystem::KillTarget(float* target) {
    for (auto& tween : m_tweens) {
        if (tween.active && tween.target == target) {
            tween.active = false;
        }
    }
}

void TweenSystem::Update(float dt) {
    for (auto& tween : m_tweens) {
        if (!tween.active || !tween.target) continue;

        tween.elapsed += dt;
        float t = std::clamp(tween.elapsed / tween.duration, 0.0f, 1.0f);
        
        float easedT = Apply(tween.easing, t);

        if (tween.pingPongDir) {
            *tween.target = tween.from + (tween.to - tween.from) * easedT;
        } else {
            *tween.target = tween.to + (tween.from - tween.to) * easedT;
        }

        if (tween.elapsed >= tween.duration) {
            if (tween.loop && tween.pingPong) {
                tween.pingPongDir = !tween.pingPongDir;
                tween.elapsed = 0.0f;
            } else if (tween.loop) {
                tween.elapsed = 0.0f;
            } else {
                *tween.target = tween.pingPongDir ? tween.to : tween.from;
                tween.active = false;
                if (tween.onComplete) {
                    tween.onComplete();
                }
            }
        }
    }
}

void TweenSystem::Clear() {
    m_tweens.clear();
}
