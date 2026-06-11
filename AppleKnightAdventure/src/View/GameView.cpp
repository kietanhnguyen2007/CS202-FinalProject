#include "View/GameView.h"
#include "View/CharacterRenderer.h"
#include "View/Renderer.h"

namespace View {

GameView& GameView::GetInstance() {
    static GameView instance;
    return instance;
}

void GameView::Init() {
    // Initialize view components
    View::Renderer::GetInstance();
}

void GameView::Update(float dt) {
    View::CharacterRenderer::GetInstance().UpdateAll(dt);
}

void GameView::Render() {
    View::CharacterRenderer::GetInstance().RenderAll();
}

void GameView::Shutdown() {
    View::CharacterRenderer::GetInstance().Clear();
}

} // namespace View
