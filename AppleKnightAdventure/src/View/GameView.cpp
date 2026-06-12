#include "View/GameView.h"
#include "View/CharacterRenderer.h"
#include "View/ParticleRenderer.h"
#include "View/Renderer.h"

namespace View {

GameView& GameView::GetInstance() {
    static GameView instance;
    return instance;
}

void GameView::Init() {
    View::Renderer::GetInstance();
    View::ParticleRenderer::GetInstance();
}

void GameView::Update(float dt) {
    View::CharacterRenderer::GetInstance().UpdateAll(dt);
}

void GameView::Render(const Camera2D& camera, const std::vector<Particle*>& particles) {
    View::Renderer& r = View::Renderer::GetInstance();

    BeginMode2D(camera);
    r.BeginFrame();

    View::CharacterRenderer::GetInstance().RenderAll();
    View::ParticleRenderer::GetInstance().RenderAll(particles);

    r.EndFrameAndFlush();
    EndMode2D();
}

void GameView::Shutdown() {
    View::CharacterRenderer::GetInstance().Clear();
    View::ParticleRenderer::GetInstance().Shutdown();
}

} // namespace View
