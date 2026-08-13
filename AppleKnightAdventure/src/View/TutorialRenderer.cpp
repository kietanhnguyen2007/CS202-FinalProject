#include "View/TutorialRenderer.h"
#include "View/Renderer.h"
#include "Model/InMapGuide.h"
#include "Model/LevelCompleteCup.h"
#include "Model/Signboard.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <unordered_set>

namespace View {

TutorialRenderer& TutorialRenderer::GetInstance() {
    static TutorialRenderer instance;
    return instance;
}

bool TutorialRenderer::Init() {
    if (m_initialized) return true;

    m_signTexture = ::LoadTexture("assets/textures/tutorial/signboard.png");
    m_cupTexture = ::LoadTexture("assets/textures/tutorial/golden_trophy.png");
    m_keyTexture = ::LoadTexture("assets/textures/tutorial/kb_dark_all.png");

    const char* ignoredGlyphText = u8"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
        u8" .,!?():-/[]'\""
        u8"BẢNG HƯỚNG DẪN Bấm F để interact đọc bảng lưu game tại Checkpoint. "
        u8"Cổng Portal sẽ dịch chuyển bạn đến cổng có cùng màu sắc. hoàn thành Tutorial Đã kích hoạt! "
        u8"Lưu Dùng Dash qua khoảng trống Kỹ năng Tấn công Ultimate";
    (void)ignoredGlyphText;
    const char* glyphText = u8"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"
        u8" .,!?():-/[]'\""
        u8"B\u1ea2NG H\u01af\u1edaNG D\u1eaaN B\u1ea5m F \u0111\u1ec3 interact \u0111\u1ecdc b\u1ea3ng "
        u8"l\u01b0u game t\u1ea1i Checkpoint. C\u1ed5ng Portal s\u1ebd d\u1ecbch chuy\u1ec3n b\u1ea1n "
        u8"\u0111\u1ebfn c\u1ed5ng c\u00f3 c\u00f9ng m\u00e0u s\u1eafc. ho\u00e0n th\u00e0nh Tutorial "
        u8"\u0110\u00e3 k\u00edch ho\u1ea1t! L\u01b0u D\u00f9ng Dash qua kho\u1ea3ng tr\u1ed1ng "
        u8"K\u1ef9 n\u0103ng T\u1ea5n c\u00f4ng Ultimate "
        u8"Khu v\u1ef1c Di chuy\u1ec3n c\u00e1c ph\u00edm l\u00e0m quen v\u1edbi \u0111i\u1ec1u khi\u1ec3n "
        u8"\u0111\u1ecdc h\u01b0\u1edbng d\u1eabn qua khu ti\u1ebfp theo L\u01b0\u1edbt h\u1ed1 "
        u8"th\u1eed ba v\u00e0 l\u00ean qu\u00e1i c\u1eadn chi\u1ebfn chi\u00eau cu\u1ed1i k\u1ebft li\u1ec5u m\u1ea1nh "
        u8"Ho\u00e0n th\u00e0nh \u0110\u1ee9ng g\u1ea7n c\u00fap v\u00e0ng nh\u1ea5n n\u00fat";
    std::vector<int> codepoints;
    std::unordered_set<int> seen;
    const char* cursor = glyphText;
    while (*cursor != '\0') {
        int byteCount = 0;
        int codepoint = ::GetCodepointNext(cursor, &byteCount);
        if (byteCount <= 0) break;
        if (seen.insert(codepoint).second) codepoints.push_back(codepoint);
        cursor += byteCount;
    }
    m_font = ::LoadFontEx("assets/fonts/tutorial_vietnamese.ttf", 32,
                          codepoints.data(), static_cast<int>(codepoints.size()));
    if (m_font.texture.id != 0) ::SetTextureFilter(m_font.texture, TEXTURE_FILTER_POINT);

    if (m_signTexture.id != 0) ::SetTextureFilter(m_signTexture, TEXTURE_FILTER_POINT);
    if (m_cupTexture.id != 0) ::SetTextureFilter(m_cupTexture, TEXTURE_FILTER_POINT);
    if (m_keyTexture.id != 0) ::SetTextureFilter(m_keyTexture, TEXTURE_FILTER_POINT);

    m_initialized = true;
    return m_signTexture.id != 0 && m_cupTexture.id != 0 && m_keyTexture.id != 0;
}

void TutorialRenderer::Shutdown() {
    if (!m_initialized) return;
    if (m_signTexture.id != 0) ::UnloadTexture(m_signTexture);
    if (m_cupTexture.id != 0) ::UnloadTexture(m_cupTexture);
    if (m_keyTexture.id != 0) ::UnloadTexture(m_keyTexture);
    if (m_font.texture.id != 0) ::UnloadFont(m_font);
    m_signTexture = {};
    m_cupTexture = {};
    m_keyTexture = {};
    m_font = {};
    m_dialogText.clear();
    m_dialogVisible = false;
    m_initialized = false;
}

bool TutorialRenderer::GetKeySource(const std::string& key, int frame, Rectangle& source) const {
    frame = std::clamp(frame, 0, 3);
    int column = frame;
    int row = -1;

    if (key.size() == 1 && key[0] >= 'A' && key[0] <= 'Z') {
        row = key[0] - 'A';
    } else {
        column = 12 + frame;
        if (key == "UP") row = 12;
        else if (key == "LEFT") row = 13;
        else if (key == "DOWN") row = 14;
        else if (key == "RIGHT") row = 15;
    }

    if (row < 0) return false;
    source = {column * 16.0f, row * 16.0f, 16.0f, 16.0f};
    return true;
}

void TutorialRenderer::RenderAll(const std::vector<std::unique_ptr<Entity>>& entities, float dt) {
    if (!m_initialized) return;
    m_time += dt;

    struct CaptionDraw {
        Vector2 pos;
        std::string text;
    };
    std::vector<CaptionDraw> captions;
    Renderer& renderer = Renderer::GetInstance();

    for (const auto& entity : entities) {
        if (!entity || !entity->IsActive()) continue;
        Vector2 pos = entity->GetPosition();

        if (entity->GetType() == EntityType::Signboard && m_signTexture.id != 0) {
            renderer.SubmitSprite(&m_signTexture,
                                  {0.0f, 0.0f, (float)m_signTexture.width, (float)m_signTexture.height},
                                  pos, {2.0f, 2.0f}, 0.0f, {}, WHITE,
                                  Layer::World, 0.15f, false,
                                  static_cast<uint32_t>(entity->GetId()));
        } else if (entity->GetType() == EntityType::LevelCompleteCup && m_cupTexture.id != 0) {
            const auto* cup = static_cast<const LevelCompleteCup*>(entity.get());
            const float bob = cup->IsActivated()
                ? -12.0f + std::sin(m_time * 7.0f) * 2.0f
                : std::sin(m_time * 2.5f) * 3.0f;
            const float buttonY = pos.y + 92.0f;
            renderer.DrawRectangle({pos.x + 17.0f, buttonY}, {62.0f, 10.0f},
                                   Color{68, 54, 48, 255}, Layer::World, 0.10f);
            renderer.DrawRectangle({pos.x + 25.0f, buttonY - (cup->IsActivated() ? 1.0f : 6.0f)},
                                   {46.0f, cup->IsActivated() ? 4.0f : 9.0f},
                                   cup->IsActivated() ? Color{80, 200, 90, 255} : Color{225, 55, 48, 255},
                                   Layer::World, 0.11f);
            renderer.SubmitSprite(&m_cupTexture,
                                  {0.0f, 0.0f, (float)m_cupTexture.width, (float)m_cupTexture.height},
                                  {pos.x, pos.y + bob}, {0.55f, 0.55f}, 0.0f, {}, WHITE,
                                  Layer::World, 0.16f, false,
                                  static_cast<uint32_t>(entity->GetId()));
        } else if (entity->GetType() == EntityType::InMapGuide) {
            const auto* guide = static_cast<const InMapGuide*>(entity.get());
            Rectangle keySource{};
            const int frame = static_cast<int>(m_time * 8.0f) % 4;
            if (m_keyTexture.id == 0 || !GetKeySource(guide->GetKey(), frame, keySource)) continue;
            const float bob = std::sin(m_time * 3.0f + entity->GetId() * 0.43f) * 4.0f;
            Vector2 iconPos{pos.x, pos.y + bob};
            renderer.SubmitSprite(&m_keyTexture, keySource,
                                  iconPos, {4.0f, 4.0f}, 0.0f, {}, WHITE,
                                  Layer::World, 0.30f, false,
                                  static_cast<uint32_t>(entity->GetId()));
            if (!guide->GetCaption().empty()) {
                captions.push_back({{iconPos.x + 32.0f, iconPos.y + 68.0f}, guide->GetCaption()});
            }
        }
    }

    if (captions.empty() || m_font.texture.id == 0) return;
    renderer.EndFrameAndFlush();
    for (const auto& caption : captions) {
        constexpr float fontSize = 15.0f;
        constexpr float spacing = 1.0f;
        Vector2 measured = ::MeasureTextEx(m_font, caption.text.c_str(), fontSize, spacing);
        Vector2 textPos{caption.pos.x - measured.x * 0.5f, caption.pos.y};
        ::DrawTextEx(m_font, caption.text.c_str(), {textPos.x + 1.0f, textPos.y + 1.0f},
                     fontSize, spacing, BLACK);
        ::DrawTextEx(m_font, caption.text.c_str(), textPos, fontSize, spacing, WHITE);
    }
}

void TutorialRenderer::ShowDialog(const std::string& text) {
    m_dialogText = text;
    m_dialogVisible = true;
}

void TutorialRenderer::HideDialog() {
    m_dialogVisible = false;
    m_dialogText.clear();
}

bool TutorialRenderer::IsDialogVisible() const { return m_dialogVisible; }

void TutorialRenderer::DrawWrappedText(const std::string& text, Rectangle bounds,
                                       float fontSize, float spacing, float lineHeight,
                                       Color color) const {
    std::istringstream input(text);
    std::string word;
    std::string line;
    float y = bounds.y;
    while (input >> word) {
        std::string candidate = line.empty() ? word : line + " " + word;
        if (!line.empty() && ::MeasureTextEx(m_font, candidate.c_str(), fontSize, spacing).x > bounds.width) {
            ::DrawTextEx(m_font, line.c_str(), {bounds.x, y}, fontSize, spacing, color);
            y += lineHeight;
            line = word;
        } else {
            line = std::move(candidate);
        }
    }
    if (!line.empty() && y <= bounds.y + bounds.height) {
        ::DrawTextEx(m_font, line.c_str(), {bounds.x, y}, fontSize, spacing, color);
    }
}

void TutorialRenderer::RenderDialog() const {
    if (!m_dialogVisible || m_font.texture.id == 0) return;
    int screenWidth = ::GetScreenWidth();
    int screenHeight = ::GetScreenHeight();
    Rectangle panel{
        screenWidth * 0.15f,
        screenHeight * 0.19f,
        screenWidth * 0.70f,
        screenHeight * 0.57f
    };
    Rectangle shadow{panel.x + 12.0f, panel.y + 14.0f, panel.width, panel.height};
    Rectangle paper{panel.x + 34.0f, panel.y + 102.0f,
                    panel.width - 68.0f, panel.height - 168.0f};
    Rectangle header{panel.x + 112.0f, panel.y + 22.0f,
                     panel.width - 224.0f, 58.0f};

    ::DrawRectangle(0, 0, screenWidth, screenHeight, Color{5, 3, 2, 175});
    ::DrawRectangleRounded(shadow, 0.055f, 12, Color{10, 6, 4, 185});
    ::DrawRectangleRounded(panel, 0.055f, 12, Color{78, 39, 20, 255});
    ::DrawRectangleRoundedLinesEx(panel, 0.055f, 12, 6.0f, Color{45, 23, 13, 255});

    for (int i = 1; i < 5; ++i) {
        float y = panel.y + panel.height * (float)i / 5.0f;
        ::DrawLineEx({panel.x + 9.0f, y}, {panel.x + panel.width - 9.0f, y},
                     3.0f, Color{55, 28, 16, 210});
        ::DrawLineEx({panel.x + 12.0f, y + 3.0f}, {panel.x + panel.width - 12.0f, y + 3.0f},
                     1.0f, Color{119, 66, 32, 190});
    }

    const Vector2 studs[] = {
        {panel.x + 20.0f, panel.y + 20.0f},
        {panel.x + panel.width - 20.0f, panel.y + 20.0f},
        {panel.x + 20.0f, panel.y + panel.height - 20.0f},
        {panel.x + panel.width - 20.0f, panel.y + panel.height - 20.0f}
    };
    for (Vector2 stud : studs) {
        ::DrawCircleV(stud, 8.0f, Color{75, 42, 17, 255});
        ::DrawCircleV(stud, 5.0f, Color{226, 170, 60, 255});
        ::DrawCircleV({stud.x - 1.5f, stud.y - 1.5f}, 1.5f, Color{255, 229, 138, 255});
    }

    ::DrawRectangleRounded(header, 0.18f, 10, Color{48, 25, 15, 255});
    ::DrawRectangleRoundedLinesEx(header, 0.18f, 10, 3.0f, Color{193, 120, 44, 255});
    if (m_signTexture.id != 0) {
        ::DrawTexturePro(m_signTexture,
                         {0.0f, 0.0f, (float)m_signTexture.width, (float)m_signTexture.height},
                         {panel.x + 30.0f, panel.y + 10.0f, 72.0f, 72.0f},
                         {0.0f, 0.0f}, 0.0f, WHITE);
    }

    ::DrawRectangleRounded(paper, 0.045f, 10, Color{239, 220, 168, 255});
    ::DrawRectangleRoundedLinesEx(paper, 0.045f, 10, 4.0f, Color{119, 70, 30, 255});
    ::DrawLineEx({paper.x + 18.0f, paper.y + 14.0f},
                 {paper.x + paper.width - 18.0f, paper.y + 14.0f},
                 2.0f, Color{205, 174, 112, 255});

    const char* title = "FIELD GUIDE";
    Vector2 titleSize = ::MeasureTextEx(m_font, title, 31.0f, 1.5f);
    ::DrawTextEx(m_font, title,
                 {header.x + (header.width - titleSize.x) * 0.5f,
                  header.y + (header.height - titleSize.y) * 0.5f - 1.0f},
                 31.0f, 1.5f, Color{255, 221, 132, 255});
    DrawWrappedText(m_dialogText,
                    {paper.x + 24.0f, paper.y + 28.0f,
                     paper.width - 48.0f, paper.height - 48.0f},
                    25.0f, 1.0f, 35.0f, Color{57, 34, 23, 255});
    const char* closeHint = "[F / ENTER / ESC] Close";
    Vector2 hintSize = ::MeasureTextEx(m_font, closeHint, 18.0f, 1.0f);
    Rectangle hintPlate{
        panel.x + panel.width - hintSize.x - 58.0f,
        panel.y + panel.height - 50.0f,
        hintSize.x + 34.0f,
        32.0f
    };
    ::DrawRectangleRounded(hintPlate, 0.25f, 8, Color{45, 23, 14, 235});
    ::DrawTextEx(m_font, closeHint,
                 {hintPlate.x + 17.0f, hintPlate.y + 6.0f},
                 18.0f, 1.0f, Color{245, 211, 126, 255});
}

} // namespace View
