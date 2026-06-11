#ifndef PARTICLESYSTEM_H
#define PARTICLESYSTEM_H

#include "raylib.h"
#include "View/Renderer.h"
#include "ObjectPool.h"
#include <vector>

struct Particle {
    Vector2 position;
    Vector2 velocity;
    Color color;
    Color startColor;
    Color endColor;
    float lifetime;
    float lifeTimer;
    float size;
    float startSize;
    float endSize;
    bool active;

    Particle();

    void Update(float deltaTime);
    void Render() const;
    float GetProgress() const;
    bool IsExpired() const;
};

class ParticleSystem {
protected:
    ObjectPool<Particle> m_pool;
    std::vector<Particle*> m_active;

public:
    ParticleSystem();
    explicit ParticleSystem(size_t initialSize);
    ~ParticleSystem();

    void Update(float deltaTime);
    void Render() const;

    void Emit(Vector2 position, Vector2 velocity, Color color,
              float lifetime, float size);
    void Emit(Vector2 position, Vector2 velocity,
              Color startColor, Color endColor,
              float lifetime, float startSize, float endSize);

    void EmitBurst(Vector2 position, int count, Color color,
                   float lifetime, float size, float spread);

    void Clear();
    size_t GetActiveCount() const;

    static void Shutdown();
};

#endif
