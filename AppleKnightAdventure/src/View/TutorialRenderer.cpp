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

namespace {
void DrawPanel(Texture2D texture, Rectangle rect, float border = 12.0f, Color tint = WHITE) {
    if (!texture.id) return;
    NPatchInfo patch{{0.0f, 0.0f, (float)texture.width, (float)texture.height},
                     (int)border, (int)border, (int)border, (int)border, NPATCH_NINE_PATCH};
    ::DrawTextureNPatch(texture, patch, rect, {0.0f, 0.0f}, 0.0f, tint);
}
}

TutorialRenderer& TutorialRenderer::GetInstance() {
    static TutorialRenderer instance;
    return instance;
}

bool TutorialRenderer::Init() {
    if (m_initialized) return true;

    m_signTexture = ::LoadTexture("assets/textures/tutorial/signboard.png");
    m_cupTexture = ::LoadTexture("assets/textures/tutorial/golden_trophy.png");
    m_keyTexture = ::LoadTexture("assets/textures/tutorial/kb_dark_all.png");
    m_panelBrown = ::LoadTexture("assets/ui/kenney_rpg/panel_brown.png");
    m_panelInsetBrown = ::LoadTexture("assets/ui/kenney_rpg/panelInset_brown.png");
    m_buttonRoundBrown = ::LoadTexture("assets/ui/kenney_rpg/buttonRound_brown.png");

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
    if (m_panelBrown.id != 0) ::SetTextureFilter(m_panelBrown, TEXTURE_FILTER_POINT);
    if (m_panelInsetBrown.id != 0) ::SetTextureFilter(m_panelInsetBrown, TEXTURE_FILTER_POINT);
    if (m_buttonRoundBrown.id != 0) ::SetTextureFilter(m_buttonRoundBrown, TEXTURE_FILTER_POINT);

    m_initialized = true;
    return m_signTexture.id != 0 && m_cupTexture.id != 0 && m_keyTexture.id != 0;
}

void TutorialRenderer::Shutdown() {
    if (!m_initialized) return;
    if (m_signTexture.id != 0) ::UnloadTexture(m_signTexture);
    if (m_cupTexture.id != 0) ::UnloadTexture(m_cupTexture);
    if (m_keyTexture.id != 0) ::UnloadTexture(m_keyTexture);
    if (m_panelBrown.id != 0) ::UnloadTexture(m_panelBrown);
    if (m_panelInsetBrown.id != 0) ::UnloadTexture(m_panelInsetBrown);
    if (m_buttonRoundBrown.id != 0) ::UnloadTexture(m_buttonRoundBrown);
    if (m_font.texture.id != 0) ::UnloadFont(m_font);
    m_signTexture = {};
    m_cupTexture = {};
    m_keyTexture = {};
    m_panelBrown = {};
    m_panelInsetBrown = {};
    m_buttonRoundBrown = {};
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
            const bool activated = cup->IsActivated();
            const float bob = activated
                ? -12.0f + std::sin(m_time * 7.0f) * 2.0f
                : std::sin(m_time * 2.5f) * 3.0f;

            // Anchor the prop to the bottom edge of the gameplay box, which is
            // the floor line the cup was placed on. Drawing from the top-left
            // corner instead left the pedestal hanging most of a tile in the
            // air, and the trophy artwork is smaller than the box it lives in.
            constexpr float cupScale = 0.55f;
            constexpr float plateHeight = 10.0f;
            const Vector2 boxSize = cup->GetSize();
            const float artWidth  = (float)m_cupTexture.width  * cupScale;
            const float artHeight = (float)m_cupTexture.height * cupScale;
            const float artX   = pos.x + (boxSize.x - artWidth) * 0.5f;
            const float plateY = pos.y + boxSize.y - plateHeight;

            renderer.DrawRectangle({artX + (artWidth - 62.0f) * 0.5f, plateY},
                                   {62.0f, plateHeight},
                                   Color{68, 54, 48, 255}, Layer::World, 0.10f);
            renderer.DrawRectangle({artX + (artWidth - 46.0f) * 0.5f,
                                    plateY - (activated ? 1.0f : 6.0f)},
                                   {46.0f, activated ? 4.0f : 9.0f},
                                   activated ? Color{80, 200, 90, 255} : Color{225, 55, 48, 255},
                                   Layer::World, 0.11f);
            renderer.SubmitSprite(&m_cupTexture,
                                  {0.0f, 0.0f, (float)m_cupTexture.width, (float)m_cupTexture.height},
                                  {artX, plateY - artHeight + bob}, {cupScale, cupScale},
                                  0.0f, {}, WHITE,
                                  Layer::World, 0.16f, false,
                                  static_cast<uint32_t>(entity->GetId()));
        } else if (entity->GetType() == EntityType::InMapGuide) {
            const auto* guide = static_cast<const InMapGuide*>(entity.get());
            Rectangle keySource{};
            const int frame = static_cast<int>(m_time * 8.0f) % 4;
            if (m_keyTexture.id == 0 || !GetKeySource(guide->GetKey(), frame, keySource)) continue;
            // Use one shared bob phase so prompts authored on the same guide
            // line remain visually aligned instead of drifting independently.
            const float bob = std::sin(m_time * 3.0f) * 4.0f;
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
        constexpr float fontSize = 22.0f;
        constexpr float spacing = 1.0f;
        Vector2 measured = ::MeasureTextEx(m_font, caption.text.c_str(), fontSize, spacing);
        const float panelWidth = std::max(104.0f, measured.x + 34.0f);
        Rectangle plate{caption.pos.x - panelWidth * 0.5f, caption.pos.y - 5.0f,
                        panelWidth, 38.0f};
        ::DrawRectangleRounded({plate.x + 3.0f, plate.y + 4.0f, plate.width, plate.height},
                               0.20f, 8, Color{12, 7, 4, 145});
        DrawPanel(m_panelBrown, plate, 12.0f);
        Vector2 textPos{caption.pos.x - measured.x * 0.5f,
                        plate.y + (plate.height - measured.y) * 0.5f - 1.0f};
        ::DrawTextEx(m_font, caption.text.c_str(), {textPos.x + 1.5f, textPos.y + 2.0f},
                     fontSize, spacing, Color{50, 24, 11, 220});
        ::DrawTextEx(m_font, caption.text.c_str(), textPos, fontSize, spacing,
                     Color{255, 239, 190, 255});
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

void TutorialRenderer::DrawWrappedCenteredText(const std::string& text, Rectangle bounds,
                                               float fontSize, float spacing, float lineHeight,
                                               Color color) const {
    std::istringstream input(text);
    std::string word;
    std::string line;
    std::vector<std::string> lines;
    while (input >> word) {
        std::string candidate = line.empty() ? word : line + " " + word;
        if (!line.empty() && ::MeasureTextEx(m_font, candidate.c_str(), fontSize, spacing).x > bounds.width) {
            lines.push_back(line);
            line = word;
        } else {
            line = std::move(candidate);
        }
    }
    if (!line.empty()) lines.push_back(line);

    const float blockHeight = lines.empty() ? 0.0f : (lines.size() - 1) * lineHeight + fontSize;
    float y = bounds.y + (bounds.height - blockHeight) * 0.5f;
    for (const std::string& centeredLine : lines) {
        Vector2 measured = ::MeasureTextEx(m_font, centeredLine.c_str(), fontSize, spacing);
        const float x = bounds.x + (bounds.width - measured.x) * 0.5f;
        ::DrawTextEx(m_font, centeredLine.c_str(), {x + 2.0f, y + 2.0f},
                     fontSize, spacing, Color{109, 67, 37, 125});
        ::DrawTextEx(m_font, centeredLine.c_str(), {x, y}, fontSize, spacing, color);
        y += lineHeight;
    }
}

void TutorialRenderer::RenderDialog() const {
    if (!m_dialogVisible || m_font.texture.id == 0) return;
    const int screenWidth = ::GetScreenWidth();
    const int screenHeight = ::GetScreenHeight();
    const float scale = std::clamp(std::min(screenWidth / 1280.0f, screenHeight / 720.0f), 0.72f, 1.4f);
    Rectangle panel{
        screenWidth * 0.12f,
        screenHeight * 0.16f,
        screenWidth * 0.76f,
        screenHeight * 0.64f
    };
    Rectangle shadow{panel.x + 12.0f*scale, panel.y + 14.0f*scale, panel.width, panel.height};
    Rectangle header{panel.x + 128.0f*scale, panel.y + 25.0f*scale,
                     panel.width - 256.0f*scale, 66.0f*scale};
    Rectangle paper{panel.x + 42.0f*scale, panel.y + 112.0f*scale,
                    panel.width - 84.0f*scale, panel.height - 184.0f*scale};

    ::DrawRectangle(0, 0, screenWidth, screenHeight, Color{5, 3, 2, 175});
    ::DrawRectangleRounded(shadow, 0.06f, 12, Color{10, 6, 4, 190});
    DrawPanel(m_panelBrown, panel, 14.0f);
    ::DrawRectangleRounded(paper, 0.035f, 10, Color{246, 226, 173, 255});
    DrawPanel(m_panelInsetBrown, paper, 14.0f, Color{255, 238, 194, 255});
    DrawPanel(m_panelBrown, header, 13.0f, Color{235, 207, 159, 255});

    // Decorative medallions make the guide feel like a finished RPG interface.
    if (m_buttonRoundBrown.id != 0) {
        const float studSize = 38.0f * scale;
        const Vector2 studs[] = {
            {panel.x + 15.0f*scale, panel.y + 15.0f*scale},
            {panel.x + panel.width - 15.0f*scale - studSize, panel.y + 15.0f*scale},
            {panel.x + 15.0f*scale, panel.y + panel.height - 15.0f*scale - studSize},
            {panel.x + panel.width - 15.0f*scale - studSize,
             panel.y + panel.height - 15.0f*scale - studSize}
        };
        for (Vector2 stud : studs) {
            ::DrawTexturePro(m_buttonRoundBrown,
                             {0,0,(float)m_buttonRoundBrown.width,(float)m_buttonRoundBrown.height},
                             {stud.x, stud.y, studSize, studSize}, {0,0}, 0.0f, WHITE);
        }
    }
    if (m_signTexture.id != 0) {
        ::DrawTexturePro(m_signTexture,
                         {0.0f, 0.0f, (float)m_signTexture.width, (float)m_signTexture.height},
                         {panel.x + 41.0f*scale, panel.y + 15.0f*scale, 82.0f*scale, 82.0f*scale},
                         {0.0f, 0.0f}, 0.0f, WHITE);
    }

    const char* title = "FIELD GUIDE";
    const float titleFontSize = 38.0f * scale;
    Vector2 titleSize = ::MeasureTextEx(m_font, title, titleFontSize, 2.0f);
    ::DrawTextEx(m_font, title,
                 {header.x + (header.width - titleSize.x) * 0.5f + 2.0f*scale,
                  header.y + (header.height - titleSize.y) * 0.5f + 2.0f*scale},
                 titleFontSize, 2.0f, Color{54, 27, 12, 190});
    ::DrawTextEx(m_font, title,
                 {header.x + (header.width - titleSize.x) * 0.5f,
                  header.y + (header.height - titleSize.y) * 0.5f - 1.0f},
                 titleFontSize, 2.0f, Color{255, 238, 185, 255});

    const float bodyFontSize = 30.0f * scale;
    DrawWrappedCenteredText(m_dialogText,
                    {paper.x + 36.0f*scale, paper.y + 22.0f*scale,
                     paper.width - 72.0f*scale, paper.height - 44.0f*scale},
                    bodyFontSize, 1.4f, 42.0f*scale, Color{62, 35, 20, 255});

    const char* closeHint = "[F / ENTER / ESC] Close";
    const float hintFontSize = 20.0f * scale;
    Vector2 hintSize = ::MeasureTextEx(m_font, closeHint, hintFontSize, 1.0f);
    Rectangle hintPlate{
        panel.x + (panel.width - hintSize.x - 38.0f*scale) * 0.5f,
        panel.y + panel.height - 56.0f*scale,
        hintSize.x + 38.0f*scale,
        36.0f*scale
    };
    DrawPanel(m_panelBrown, hintPlate, 11.0f, Color{215, 191, 150, 255});
    ::DrawTextEx(m_font, closeHint,
                 {hintPlate.x + (hintPlate.width - hintSize.x) * 0.5f,
                  hintPlate.y + (hintPlate.height - hintSize.y) * 0.5f},
                 hintFontSize, 1.0f, Color{255, 239, 194, 255});
}

} // namespace View
