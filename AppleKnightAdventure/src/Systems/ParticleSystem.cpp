#include "Systems/ParticleSystem.h"
#include "Utils/Constants.h"
#include <cmath>
#include <cstdlib>

Particle::Particle()
    : position({0, 0})
    , velocity({0, 0})
    , color(WHITE)
    , startColor(WHITE)
    , endColor({0, 0, 0, 0})
    , lifetime(PARTICLE_LIFETIME)
    , lifeTimer(0.0f)
    , size(4.0f)
    , startSize(4.0f)
    , endSize(0.0f)
    , active(false)
{
}

void Particle::Update(float deltaTime) {
    if (!active) return;
    position.x += velocity.x * deltaTime;
    position.y += velocity.y * deltaTime;
    lifeTimer += deltaTime;
    if (lifeTimer >= lifetime) {
        active = false;
        return;
    }
    float t = GetProgress();
    color.r = static_cast<unsigned char>(startColor.r + (endColor.r - startColor.r) * t);
    color.g = static_cast<unsigned char>(startColor.g + (endColor.g - startColor.g) * t);
    color.b = static_cast<unsigned char>(startColor.b + (endColor.b - startColor.b) * t);
    color.a = static_cast<unsigned char>(startColor.a + (endColor.a - startColor.a) * t);
    size = startSize + (endSize - startSize) * t;
}

float Particle::GetProgress() const {
    return (lifetime > 0.0f) ? lifeTimer / lifetime : 1.0f;
}

bool Particle::IsExpired() const {
    return !active || lifeTimer >= lifetime;
}

ParticleSystem::ParticleSystem()
    : m_pool(256)
{
    m_active.reserve(256);
}

ParticleSystem::ParticleSystem(size_t initialSize)
    : m_pool(initialSize)
{
    m_active.reserve(initialSize);
}

ParticleSystem::~ParticleSystem() {
}

void ParticleSystem::Update(float deltaTime) {
    for (auto it = m_active.begin(); it != m_active.end(); ) {
        (*it)->Update(deltaTime);
        if ((*it)->IsExpired()) {
            m_pool.Release(*it);
            it = m_active.erase(it);
        } else {
            ++it;
        }
    }
}

void ParticleSystem::Emit(Vector2 position, Vector2 velocity, Color color,
                          float lifetime, float size) {
    Particle* p = m_pool.Acquire();
    p->position = position;
    p->velocity = velocity;
    p->color = color;
    p->startColor = color;
    p->endColor = {color.r, color.g, color.b, 0};
    p->lifetime = lifetime;
    p->lifeTimer = 0.0f;
    p->size = size;
    p->startSize = size;
    p->endSize = 0.0f;
    p->active = true;
    m_active.push_back(p);
}

void ParticleSystem::Emit(Vector2 position, Vector2 velocity,
                          Color startColor, Color endColor,
                          float lifetime, float startSize, float endSize) {
    Particle* p = m_pool.Acquire();
    p->position = position;
    p->velocity = velocity;
    p->color = startColor;
    p->startColor = startColor;
    p->endColor = endColor;
    p->lifetime = lifetime;
    p->lifeTimer = 0.0f;
    p->size = startSize;
    p->startSize = startSize;
    p->endSize = endSize;
    p->active = true;
    m_active.push_back(p);
}

void ParticleSystem::EmitBurst(Vector2 position, int count, Color color,
                               float lifetime, float size, float spread) {
    for (int i = 0; i < count; ++i) {
        float angle = (static_cast<float>(std::rand()) / RAND_MAX) * 2.0f * PI;
        float speed = (static_cast<float>(std::rand()) / RAND_MAX) * spread;
        Vector2 velocity = {std::cos(angle) * speed, std::sin(angle) * speed};
        Emit(position, velocity, color, lifetime, size);
    }
}

void ParticleSystem::Clear() {
    m_pool.ReleaseAll();
    m_active.clear();
}

size_t ParticleSystem::GetActiveCount() const {
    return m_active.size();
}

void ParticleSystem::Shutdown() {
}
