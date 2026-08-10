#pragma once

#include <vector>
#include <functional>

enum class EasingType {
    Linear,
    InOutQuad,
    OutQuad,
    OutCubic,
    OutBack,
    OutBounce,
    OutElastic,
    InOutSine
};

struct Tween {
    float* target = nullptr;   // pointer to float to animate
    float from = 0.0f;
    float to = 1.0f;
    float duration = 1.0f;
    float elapsed = 0.0f;
    EasingType easing = EasingType::Linear;
    bool loop = false;
    bool pingPong = false;
    bool active = false;
    bool pingPongDir = true;  // true=forward
    std::function<void()> onComplete;  // optional callback
};

class TweenSystem {
public:
    static TweenSystem& GetInstance();

    Tween& Add(float* target, float from, float to, float duration, EasingType easing, bool loop = false, bool pingPong = false);
    void KillTarget(float* target);
    void Update(float dt);
    void Clear();

    static float Apply(EasingType e, float t);

private:
    TweenSystem() = default;
    ~TweenSystem() = default;

    std::vector<Tween> m_tweens;
};
