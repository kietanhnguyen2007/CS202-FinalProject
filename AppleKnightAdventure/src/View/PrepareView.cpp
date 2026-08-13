#include "View/PrepareView.h"
#include "Controller/PrepareController.h"
#include "Systems/SoundManager.h"
#include "View/Animator.h"
#include "View/TextureAtlas.h"
#include <raylib.h>
#include <algorithm>
#include <cmath>

namespace View {
namespace {

struct PrepareLayout {
    Rectangle back{};
    Rectangle characters{};
    Rectangle preview{};
    Rectangle pets{};
    Rectangle start{};
};

PrepareLayout BuildPrepareLayout() {
    const float sw = static_cast<float>(GetScreenWidth());
    const float sh = static_cast<float>(GetScreenHeight());
    const float margin = std::clamp(sw * 0.025f, 12.0f, 32.0f);
    const float gap = std::clamp(sw * 0.018f, 8.0f, 22.0f);
    const float sideW = std::clamp(sw * 0.245f, 180.0f, 306.0f);
    const float top = std::clamp(sh * 0.17f, 78.0f, 125.0f);
    const float bottomReserve = std::clamp(sh * 0.18f, 76.0f, 122.0f);
    const float panelH = std::max(170.0f, sh - top - bottomReserve);

    PrepareLayout l;
    l.back = {margin, 18.0f, std::clamp(sw * 0.09f, 92.0f, 120.0f), 40.0f};
    l.characters = {margin, top, sideW, panelH};
    l.pets = {sw - margin - sideW, top, sideW, panelH};
    l.preview = {l.characters.x + l.characters.width + gap, top,
                 std::max(180.0f, l.pets.x - gap - (l.characters.x + l.characters.width + gap)),
                 panelH};
    const float startW = std::clamp(l.preview.width * 0.62f, 170.0f, 270.0f);
    const float startH = std::clamp(sh * 0.075f, 46.0f, 62.0f);
    l.start = {l.preview.x + (l.preview.width - startW) * 0.5f,
               sh - bottomReserve * 0.70f, startW, startH};
    return l;
}

Rectangle ItemRect(Rectangle panel, int row, int count) {
    const float top = panel.y + 49.0f;
    const float gap = 7.0f;
    const float available = panel.height - 62.0f;
    const float h = std::max(24.0f, (available - gap * (count - 1)) / count);
    return {panel.x + 10.0f, top + row * (h + gap), panel.width - 20.0f, h};
}

} // namespace

PrepareView& PrepareView::GetInstance() {
    static PrepareView instance;
    return instance;
}

bool PrepareView::Init() {
    m_backBtnAnim = {};
    m_startBtnAnim = {};
    m_pulseTime = 0.0f;
    if (!m_fontLoaded && FileExists("assets/fonts/game_font.ttf")) {
        m_font = LoadFont("assets/fonts/game_font.ttf");
        m_fontLoaded = m_font.texture.id != 0;
    }
    return true;
}

void PrepareView::Shutdown() {
    m_charAtlas.reset();
    m_petAtlas.reset();
    if (m_fontLoaded) {
        UnloadFont(m_font);
        m_font = {};
        m_fontLoaded = false;
    }
}

void PrepareView::Show() {
    m_backBtnAnim = {};
    m_startBtnAnim = {};
    m_pulseTime = 0.0f;
    m_loadedCharPath.clear();
    m_loadedPetPath.clear();
}

void PrepareView::Update(float dt) {
    m_pulseTime += dt;
    const PrepareLayout l = BuildPrepareLayout();
    const Vector2 mouse = GetMousePosition();
    const bool clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    auto& ctrl = PrepareController::GetInstance();
    auto& sound = SoundManager::GetInstance();

    m_backBtnAnim.hovered = CheckCollisionPointRec(mouse, l.back);
    m_startBtnAnim.hovered = CheckCollisionPointRec(mouse, l.start);
    if (m_backBtnAnim.hovered && clicked) {
        sound.PlaySound("ui_confirm");
        ctrl.SetWantsBack();
    }
    if (m_startBtnAnim.hovered) {
        if (clicked) {
            ctrl.SetFocusColumn(2);
            ctrl.TryStartGame();
        }
    }

    if (CheckCollisionPointRec(mouse, l.characters)) {
        const int count = static_cast<int>(ctrl.GetCharItems().size());
        for (int i = 0; i < count; ++i) {
            if (clicked && CheckCollisionPointRec(mouse, ItemRect(l.characters, i, count))) {
                ctrl.SetFocusColumn(0);
                sound.PlaySound(ctrl.SetSelectedCharIdx(i) ? "ui_confirm" : "ui_error");
            }
        }
    }
    if (CheckCollisionPointRec(mouse, l.pets)) {
        const int count = static_cast<int>(ctrl.GetPetItems().size()) + 1;
        for (int row = 0; row < count; ++row) {
            if (clicked && CheckCollisionPointRec(mouse, ItemRect(l.pets, row, count))) {
                ctrl.SetFocusColumn(1);
                sound.PlaySound(ctrl.SetSelectedPetIdx(row - 1) ? "ui_confirm" : "ui_error");
            }
        }
    }

    const int cIdx = ctrl.GetSelectedCharIdx();
    const int pIdx = ctrl.GetSelectedPetIdx();
    const std::string charPath = cIdx >= 0 ? ctrl.GetCharItems()[cIdx].idlePath : "";
    const std::string petPath = pIdx >= 0 ? ctrl.GetPetItems()[pIdx].idlePath : "";
    LoadPreview(charPath, petPath);
    m_charAnim.Update(dt);
    m_petAnim.Update(dt);
}

void PrepareView::Render() {
    const PrepareLayout l = BuildPrepareLayout();
    const Font font = m_fontLoaded ? m_font : GetFontDefault();
    const float sw = static_cast<float>(GetScreenWidth());
    const float sh = static_cast<float>(GetScreenHeight());
    auto& ctrl = PrepareController::GetInstance();

    DrawRectangleGradientV(0, 0, (int)sw, (int)sh, Color{12, 8, 28, 255}, Color{4, 3, 12, 255});
    DrawCircleGradient({sw * 0.5f, sh * 0.47f}, sw * 0.38f,
                       Color{96, 58, 145, 42}, Color{5, 3, 14, 0});

    const float titleSize = std::clamp(sh * 0.055f, 23.0f, 40.0f);
    const std::string title = ctrl.GetLevelId() < 0
        ? "PREPARE FOR CUSTOM MAP"
        : "PREPARE FOR LEVEL " + std::to_string(ctrl.GetLevelId());
    const Vector2 titleMeasure = MeasureTextEx(font, title.c_str(), titleSize, 1.5f);
    DrawTextEx(font, title.c_str(), {(sw - titleMeasure.x) * 0.5f, 24.0f}, titleSize, 1.5f,
               Color{255, 220, 104, 255});
    const char* subtitle = "CHOOSE YOUR HERO AND COMPANION";
    const float subSize = std::clamp(sh * 0.022f, 11.0f, 16.0f);
    const Vector2 subMeasure = MeasureTextEx(font, subtitle, subSize, 1.0f);
    DrawTextEx(font, subtitle, {(sw - subMeasure.x) * 0.5f, 24.0f + titleSize + 4.0f},
               subSize, 1.0f, Color{174, 156, 203, 230});

    RenderGrid(0, l.characters.x, l.characters.y, l.characters.width, l.characters.height);
    RenderPreview(l.preview.x, l.preview.y, l.preview.width, l.preview.height);
    RenderGrid(1, l.pets.x, l.pets.y, l.pets.width, l.pets.height);
    RenderStartButton(l.start.x, l.start.y, l.start.width, l.start.height);

    const bool backFocus = m_backBtnAnim.hovered;
    DrawRectangleRounded(l.back, 0.22f, 8, backFocus ? Color{74, 51, 106, 255} : Color{32, 24, 53, 245});
    DrawRectangleRoundedLinesEx(l.back, 0.22f, 8, backFocus ? 2.0f : 1.0f,
                                backFocus ? Color{255, 215, 100, 255} : Color{122, 94, 153, 210});
    const char* back = "< BACK";
    const float backSize = 13.0f;
    const Vector2 backMeasure = MeasureTextEx(font, back, backSize, 1.0f);
    DrawTextEx(font, back, {l.back.x + (l.back.width - backMeasure.x) * 0.5f,
                            l.back.y + (l.back.height - backMeasure.y) * 0.5f},
               backSize, 1.0f, RAYWHITE);

    const char* hint = "A/D: CHANGE PANEL     W/S: CHOOSE     ENTER: CONFIRM     ESC: BACK";
    const float hintSize = std::clamp(sh * 0.019f, 9.0f, 13.0f);
    const Vector2 hintMeasure = MeasureTextEx(font, hint, hintSize, 1.0f);
    DrawTextEx(font, hint, {(sw - hintMeasure.x) * 0.5f, sh - hintSize - 9.0f},
               hintSize, 1.0f, Color{151, 137, 178, 210});
}

void PrepareView::RenderGrid(int col, float x, float y, float w, float h) {
    auto& ctrl = PrepareController::GetInstance();
    const Font font = m_fontLoaded ? m_font : GetFontDefault();
    const bool focused = ctrl.GetFocusColumn() == col;
    const Rectangle panel = {x, y, w, h};
    const Color accent = col == 0 ? Color{104, 190, 245, 255} : Color{214, 133, 244, 255};

    DrawRectangleRounded({x + 5, y + 7, w, h}, 0.055f, 10, Color{0, 0, 0, 115});
    DrawRectangleRounded(panel, 0.055f, 10, Color{24, 18, 43, 247});
    DrawRectangleRoundedLinesEx(panel, 0.055f, 10, focused ? 3.0f : 1.5f,
                                focused ? Color{255, 215, 95, 255} : Color{101, 76, 139, 220});
    DrawRectangleGradientV((int)x + 2, (int)y + 2, (int)w - 4, 42,
                           Fade(accent, focused ? 0.30f : 0.17f), Color{24, 18, 43, 0});

    const char* heading = col == 0 ? "HERO" : "COMPANION";
    const float headingSize = std::clamp(w * 0.072f, 12.0f, 19.0f);
    const Vector2 headingMeasure = MeasureTextEx(font, heading, headingSize, 1.0f);
    DrawTextEx(font, heading, {x + (w - headingMeasure.x) * 0.5f, y + 14.0f},
               headingSize, 1.0f, focused ? Color{255, 226, 130, 255} : Color{213, 201, 232, 255});

    if (col == 0) {
        const auto& items = ctrl.GetCharItems();
        const int count = static_cast<int>(items.size());
        for (int i = 0; i < count; ++i) {
            const Rectangle card = ItemRect(panel, i, count);
            const bool selected = ctrl.GetSelectedCharIdx() == i;
            DrawRectangleRounded(card, 0.14f, 7,
                selected ? Color{64, 52, 96, 255} : Color{33, 27, 53, 245});
            DrawRectangleRoundedLinesEx(card, 0.14f, 7, selected ? 2.0f : 1.0f,
                selected ? Color{255, 211, 83, 255} : Color{91, 73, 118, 190});
            const float fs = std::clamp(card.height * 0.31f, 10.0f, 16.0f);
            DrawTextEx(font, items[i].displayName.c_str(), {card.x + 13, card.y + (card.height - fs) * 0.5f},
                       fs, 1.0f, items[i].isUnlocked ? RAYWHITE : Color{113, 104, 127, 255});
            if (selected) DrawCircleV({card.x + card.width - 17, card.y + card.height * 0.5f}, 5.0f, Color{255, 215, 83, 255});
            if (!items[i].isUnlocked) DrawLockOverlay(card.x, card.y, card.width, card.height);
        }
    } else {
        const auto& items = ctrl.GetPetItems();
        const int count = static_cast<int>(items.size()) + 1;
        for (int row = 0; row < count; ++row) {
            const int index = row - 1;
            const Rectangle card = ItemRect(panel, row, count);
            const bool selected = ctrl.GetSelectedPetIdx() == index;
            DrawRectangleRounded(card, 0.14f, 7,
                selected ? Color{70, 47, 91, 255} : Color{33, 27, 53, 245});
            DrawRectangleRoundedLinesEx(card, 0.14f, 7, selected ? 2.0f : 1.0f,
                selected ? Color{255, 211, 83, 255} : Color{91, 73, 118, 190});
            const std::string name = index < 0 ? "No Companion" : items[index].displayName;
            const bool unlocked = index < 0 || items[index].isUnlocked;
            const float fs = std::clamp(card.height * 0.29f, 9.0f, 15.0f);
            DrawTextEx(font, name.c_str(), {card.x + 13, card.y + (card.height - fs) * 0.5f},
                       fs, 1.0f, unlocked ? RAYWHITE : Color{113, 104, 127, 255});
            if (selected) DrawCircleV({card.x + card.width - 17, card.y + card.height * 0.5f}, 5.0f, Color{255, 215, 83, 255});
            if (!unlocked) DrawLockOverlay(card.x, card.y, card.width, card.height);
        }
    }
}

void PrepareView::RenderPreview(float x, float y, float w, float h) {
    auto& ctrl = PrepareController::GetInstance();
    const Font font = m_fontLoaded ? m_font : GetFontDefault();
    const bool focused = ctrl.GetFocusColumn() == 2;
    const Rectangle panel = {x, y, w, h};
    DrawRectangleRounded(panel, 0.055f, 10, Color{14, 12, 29, 205});
    DrawRectangleRoundedLinesEx(panel, 0.055f, 10, focused ? 2.5f : 1.0f,
                                focused ? Color{255, 215, 95, 230} : Color{78, 62, 106, 165});
    DrawCircleGradient({x + w * 0.5f, y + h * 0.48f}, w * 0.34f,
                       Color{128, 82, 190, 72}, Color{18, 13, 35, 0});
    DrawEllipse((int)(x + w * 0.54f), (int)(y + h * 0.76f), w * 0.27f, h * 0.035f, Fade(BLACK, 0.62f));

    const float characterScale = std::clamp(std::min(w, h) / 105.0f, 1.6f, 3.4f);
    const Vector2 characterPos = {x + w * 0.57f, y + h * 0.58f};
    if (m_charAtlas && m_charAtlas->IsTextureLoaded() && m_charAnim.HasTexture()) {
        const Rectangle src = m_charAnim.GetCurrentSrcRect();
        Texture2D* texture = m_charAnim.GetCurrentTexture();
        if (texture) {
            const float fw = fabsf(src.width) * characterScale;
            const float fh = fabsf(src.height) * characterScale;
            DrawTexturePro(*texture, src, {characterPos.x - fw * 0.5f, characterPos.y - fh * 0.5f, fw, fh}, {0,0}, 0, WHITE);
        }
    }
    if (m_petAtlas && m_petAtlas->IsTextureLoaded() && m_petAnim.HasTexture()) {
        const Rectangle src = m_petAnim.GetCurrentSrcRect();
        Texture2D* texture = m_petAnim.GetCurrentTexture();
        if (texture) {
            const float scale = characterScale * 0.48f;
            const float fw = fabsf(src.width) * scale;
            const float fh = fabsf(src.height) * scale;
            const Vector2 petPos = {x + w * 0.25f, y + h * 0.62f + sinf(m_pulseTime * 2.2f) * 4.0f};
            DrawTexturePro(*texture, src, {petPos.x - fw * 0.5f, petPos.y - fh * 0.5f, fw, fh}, {0,0}, 0, WHITE);
        }
    }

    const auto& chars = ctrl.GetCharItems();
    const int c = ctrl.GetSelectedCharIdx();
    const int p = ctrl.GetSelectedPetIdx();
    const std::string heroName = c >= 0 && c < (int)chars.size() ? chars[c].displayName : "Knight";
    const std::string petName = p >= 0 && p < (int)ctrl.GetPetItems().size()
        ? ctrl.GetPetItems()[p].displayName : "No Companion";
    const float nameSize = std::clamp(w * 0.055f, 12.0f, 21.0f);
    const Vector2 nameMeasure = MeasureTextEx(font, heroName.c_str(), nameSize, 1.0f);
    DrawTextEx(font, heroName.c_str(), {x + (w - nameMeasure.x) * 0.5f, y + 18.0f},
               nameSize, 1.0f, Color{255, 224, 132, 255});
    const float petSize = std::max(9.0f, nameSize * 0.62f);
    const std::string companion = "Companion: " + petName;
    const Vector2 petMeasure = MeasureTextEx(font, companion.c_str(), petSize, 1.0f);
    DrawTextEx(font, companion.c_str(), {x + (w - petMeasure.x) * 0.5f, y + h - petSize - 18.0f},
               petSize, 1.0f, Color{174, 159, 202, 235});
}

void PrepareView::RenderStartButton(float x, float y, float w, float h) {
    auto& ctrl = PrepareController::GetInstance();
    const Font font = m_fontLoaded ? m_font : GetFontDefault();
    const bool selected = ctrl.GetFocusColumn() == 2 || m_startBtnAnim.hovered;
    const float pulse = selected ? 1.0f + sinf(m_pulseTime * 4.5f) * 0.025f : 1.0f;
    Rectangle rect = {x - w * (pulse - 1.0f) * 0.5f, y - h * (pulse - 1.0f) * 0.5f, w * pulse, h * pulse};
    if (selected) DrawRectangleRounded({rect.x - 8, rect.y - 8, rect.width + 16, rect.height + 16}, 0.25f, 9, Color{255, 190, 62, 42});
    DrawRectangleGradientV((int)rect.x, (int)rect.y, (int)rect.width, (int)rect.height,
                           selected ? Color{112, 72, 149, 255} : Color{64, 45, 91, 255},
                           selected ? Color{61, 39, 90, 255} : Color{37, 28, 58, 255});
    DrawRectangleRoundedLinesEx(rect, 0.20f, 9, selected ? 3.0f : 1.5f,
                                selected ? Color{255, 220, 104, 255} : Color{126, 96, 158, 230});
    const char* text = "BEGIN ADVENTURE";
    const float size = std::clamp(h * 0.31f, 12.0f, 19.0f);
    const Vector2 measure = MeasureTextEx(font, text, size, 1.0f);
    DrawTextEx(font, text, {rect.x + (rect.width - measure.x) * 0.5f, rect.y + (rect.height - measure.y) * 0.5f},
               size, 1.0f, selected ? Color{255, 238, 174, 255} : RAYWHITE);
}

void PrepareView::DrawLockOverlay(float x, float y, float w, float h) {
    const Font font = m_fontLoaded ? m_font : GetFontDefault();
    DrawRectangleRounded({x, y, w, h}, 0.14f, 7, Color{8, 7, 15, 185});
    const char* text = "LOCKED";
    const float size = std::clamp(h * 0.27f, 8.0f, 12.0f);
    const Vector2 measure = MeasureTextEx(font, text, size, 1.0f);
    DrawTextEx(font, text, {x + w - measure.x - 10.0f, y + (h - measure.y) * 0.5f},
               size, 1.0f, Color{211, 112, 121, 245});
}

void PrepareView::LoadPreview(const std::string& charPath, const std::string& petPath) {
    if (charPath != m_loadedCharPath) {
        m_loadedCharPath = charPath;
        if (!charPath.empty()) {
            m_charAtlas = Animations::TextureAtlas::LoadFromJSON(charPath);
            if (m_charAtlas && m_charAtlas->LoadTexture()) {
                m_charAnim.LoadClipsFromAtlas(*m_charAtlas);
                m_charAnim.Play("idle", true);
            }
        } else m_charAtlas.reset();
    }
    if (petPath != m_loadedPetPath) {
        m_loadedPetPath = petPath;
        if (!petPath.empty()) {
            m_petAtlas = Animations::TextureAtlas::LoadFromJSON(petPath);
            if (m_petAtlas && m_petAtlas->LoadTexture()) {
                m_petAnim.LoadClipsFromAtlas(*m_petAtlas);
                m_petAnim.Play("idle", true);
            }
        } else m_petAtlas.reset();
    }
}

} // namespace View
