#include "View/GameView.h"
#include "View/CharacterRenderer.h"
#include "View/EntityRenderer.h"
#include "View/ParticleRenderer.h"
#include "View/Renderer.h"
#include "View/HUDView.h"
#include "View/FloatingText.h"
#include "View/EnemyStatusRenderer.h"
#include "View/ResultView.h"
#include "View/MenuView.h"
#include "View/InventoryView.h"
#include <cmath>

namespace View {

GameView& GameView::GetInstance() {
    static GameView instance;
    return instance;
}

void GameView::Init() {
    View::Renderer::GetInstance();
    View::ParticleRenderer::GetInstance();
    LoadShadowShader();
}

void GameView::LoadShadowShader() {
    const char* vsPath = "assets/shaders/shadow.vs";
    const char* fsPath = "assets/shaders/shadow.fs";
    // raylib LoadShader returns a valid shader even if files don't exist (id=0)
    m_shadowShader = LoadShader(vsPath, fsPath);
    if (m_shadowShader.id != 0) {
        m_shaderLoaded = true;
    }
}

void GameView::Update(float dt) {
    View::CharacterRenderer::GetInstance().UpdateAll(dt);
    View::FloatingTextManager::GetInstance().Update(dt);
    if (View::ResultView::GetInstance().IsVisible()) {
        View::ResultView::GetInstance().Update(dt);
    }
}

void GameView::SetActiveWorldLayer(WorldLayer layer, const Vector2& lightWorldPos) {
    m_activeLayer = layer;
    m_lightWorldPos = lightWorldPos;
}

WorldLayer GameView::GetActiveWorldLayer() const {
    return m_activeLayer;
}

void GameView::Render(const Camera2D& camera, const std::vector<Particle*>& particles, float dt) {
    View::Renderer& r = View::Renderer::GetInstance();

    // Apply shadow shader for Shadow world layer
    if (m_activeLayer == WorldLayer::Shadow && m_shaderLoaded) {
        // Convert light world position to screen coordinates
        Vector2 screenPos = GetWorldToScreen2D(m_lightWorldPos, camera);
        SetShaderValue(m_shadowShader, GetShaderLocation(m_shadowShader, "lightScreenPos"), &screenPos, SHADER_UNIFORM_VEC2);
        float screenW = (float)r.GetWindowWidth();
        float screenH = (float)r.GetWindowHeight();
        SetShaderValue(m_shadowShader, GetShaderLocation(m_shadowShader, "screenW"), &screenW, SHADER_UNIFORM_FLOAT);
        SetShaderValue(m_shadowShader, GetShaderLocation(m_shadowShader, "screenH"), &screenH, SHADER_UNIFORM_FLOAT);
        float radius = 150.0f;
        SetShaderValue(m_shadowShader, GetShaderLocation(m_shadowShader, "radius"), &radius, SHADER_UNIFORM_FLOAT);
        BeginShaderMode(m_shadowShader);
    }

    BeginMode2D(camera);
    r.BeginFrame();

    View::CharacterRenderer::GetInstance().RenderAll();
    View::EntityRenderer::GetInstance().RenderAll();
    View::ParticleRenderer::GetInstance().RenderAll(particles, camera, dt);
    View::EnemyStatusRenderer::GetInstance().Render(camera);
    View::FloatingTextManager::GetInstance().Render(camera);

    r.EndFrameAndFlush();
    EndMode2D();

    if (m_activeLayer == WorldLayer::Shadow && m_shaderLoaded) {
        EndShaderMode();
    }
}

void GameView::Shutdown() {
    View::CharacterRenderer::GetInstance().Clear();
    View::ParticleRenderer::GetInstance().Shutdown();
    if (m_shaderLoaded && m_shadowShader.id != 0) {
        UnloadShader(m_shadowShader);
        m_shaderLoaded = false;
    }
}

} // namespace View
