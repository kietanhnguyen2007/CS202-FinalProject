#include "Systems/ParticleSystem.h"
#include "Utils/Constants.h"
#include <cmath>
#include <cstdlib>

static Texture2D CreateSoftCircleTexture() {
    const int size = 16;
    Image img = GenImageColor(size, size, BLANK);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float dx = (x + 0.5f) / size - 0.5f;
            float dy = (y + 0.5f) / size - 0.5f;
            float dist = sqrtf(dx*dx + dy*dy) * 2.0f;
            unsigned char a = (unsigned char)((1.0f - fminf(dist, 1.0f)) * 255);
            Color c = {255, 255, 255, a};
            ImageDrawPixel(&img, x, y, c);
        }
    }
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

static Texture2D s_softCircle = {0};
static bool s_softCircleLoaded = false;
static int s_softCircleRefCount = 0;

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

void Particle::Render() const {
    if (!active) return;
    if (s_softCircleLoaded) {
        Rectangle src = {0, 0, (float)s_softCircle.width, (float)s_softCircle.height};
        Vector2 scale = {size * 2.0f / src.width, size * 2.0f / src.height};
        View::Renderer::GetInstance().SubmitSprite(
            &s_softCircle, src, position, scale, 0.0f,
            {src.width * 0.5f, src.height * 0.5f}, color,
            View::Layer::Foreground, 0.0f, false, 0);
    } else {
        DrawCircle((int)position.x, (int)position.y, size, color);
    }
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
    if (!s_softCircleLoaded) {
        s_softCircle = CreateSoftCircleTexture();
        s_softCircleLoaded = true;
    }
    s_softCircleRefCount++;
}

ParticleSystem::ParticleSystem(size_t initialSize)
    : m_pool(initialSize)
{
    m_active.reserve(initialSize);
    if (!s_softCircleLoaded) {
        s_softCircle = CreateSoftCircleTexture();
        s_softCircleLoaded = true;
    }
    s_softCircleRefCount++;
}

ParticleSystem::~ParticleSystem() {
    s_softCircleRefCount--;
    if (s_softCircleRefCount <= 0 && s_softCircleLoaded) {
        UnloadTexture(s_softCircle);
        s_softCircleLoaded = false;
    }
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
    if (s_softCircleLoaded) {
        UnloadTexture(s_softCircle);
        s_softCircleLoaded = false;
    }
    s_softCircleRefCount = 0;
}
