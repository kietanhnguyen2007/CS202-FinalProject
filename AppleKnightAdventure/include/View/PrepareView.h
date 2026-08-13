#pragma once
#include "View/UIHelpers.h"
#include "View/Animator.h"
#include "View/TextureAtlas.h"
#include <memory>
#include <string>

namespace View {
class PrepareView {
public:
    static PrepareView& GetInstance();
    bool Init();
    void Shutdown();
    void Show();
    void Update(float dt);
    void Render();

private:
    PrepareView() = default;
    ~PrepareView() = default;
    
    void RenderGrid(int col, float x, float y, float w, float h);
    void RenderPreview(float x, float y, float w, float h);
    void RenderStartButton(float x, float y, float w, float h);
    void DrawLockOverlay(float x, float y, float w, float h);
    
    void LoadPreview(const std::string& charPath, const std::string& petPath);

    std::shared_ptr<Animations::TextureAtlas> m_charAtlas;
    Animations::Animator m_charAnim;
    std::string m_loadedCharPath;

    std::shared_ptr<Animations::TextureAtlas> m_petAtlas;
    Animations::Animator m_petAnim;
    std::string m_loadedPetPath;

    BtnAnim m_backBtnAnim;
    BtnAnim m_startBtnAnim;
    float m_pulseTime = 0.f;
    Font m_font{};
    bool m_fontLoaded = false;
};
} // namespace View
