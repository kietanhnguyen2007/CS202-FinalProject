#include "View/ParticleRenderer.h"
#include "Systems/ParticleSystem.h"
#include <cmath>

static Texture2D CreateSoftCircleTexture() {
    const int size = 16;
    Image img = GenImageColor(size, size, BLANK);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float dx = (x + 0.5f) / size - 0.5f;
            float dy = (y + 0.5f) / size - 0.5f;
            float dist = sqrtf(dx*dx + dy*dy) * 2.0f;
            unsigned char a = static_cast<unsigned char>((1.0f - fminf(dist, 1.0f)) * 255);
            Color c = {255, 255, 255, a};
            ImageDrawPixel(&img, x, y, c);
        }
    }
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

namespace View {

ParticleRenderer& ParticleRenderer::GetInstance() {
    static ParticleRenderer instance;
    return instance;
}

ParticleRenderer::ParticleRenderer() {
    m_softCircle = CreateSoftCircleTexture();
    m_initialized = true;
}

ParticleRenderer::~ParticleRenderer() {
    Shutdown();
}

void ParticleRenderer::RenderAll(const std::vector<Particle*>& particles) {
    if (!m_initialized) return;

    Rectangle src = {0, 0, static_cast<float>(m_softCircle.width),
                     static_cast<float>(m_softCircle.height)};

    for (const Particle* p : particles) {
        if (!p || !p->active) continue;

        float scaleX = p->size * 2.0f / src.width;
        float scaleY = p->size * 2.0f / src.height;

        Renderer::GetInstance().SubmitSprite(
            &m_softCircle, src, p->position,
            {scaleX, scaleY}, 0.0f,
            {src.width * 0.5f, src.height * 0.5f},
            p->color, Layer::Foreground, 0.0f, false, 0);
    }
}

void ParticleRenderer::Shutdown() {
    if (m_initialized && m_softCircle.id != 0) {
        UnloadTexture(m_softCircle);
        m_softCircle = {};
    }
    m_initialized = false;
}

} // namespace View
