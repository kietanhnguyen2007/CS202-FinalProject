#include "View/MapBuilderView.h"
#include "View/AssetManager.h"
#include "View/Renderer.h"
#include "View/GameView.h"
#include "View/TextureAtlas.h"
#include "Utils/Constants.h"
#include "Model/TriggerZone.h"
#include "Model/Chest.h"
#include "Model/Boss.h"
#include "Model/Checkpoint.h"
#include "Model/Item.h"
#include "Controller/MapBuilderController.h"
#include "Systems/SoundManager.h"
#include <iostream>
#include <algorithm>

namespace View {
namespace {

struct BuilderLayout {
    float toolbarH{};
    float leftW{};
    float rightW{};
    Rectangle toolbar{};
    Rectangle palette{};
    Rectangle layers{};
    Rectangle properties{};
    Rectangle minimap{};
    Rectangle shortcut{};
    Rectangle save{};
    Rectangle fileName{};
    Rectangle load{};
    Rectangle test{};
    Rectangle clear{};
    Rectangle exit{};
    Rectangle tools[6]{};
    Rectangle widthMinus{}, widthPlus{}, heightMinus{}, heightPlus{};
};

BuilderLayout BuildBuilderLayout(float sw, float sh) {
    BuilderLayout l;
    l.toolbarH = 84.0f;
    l.leftW = std::clamp(sw * 0.19f, 188.0f, 246.0f);
    l.rightW = std::clamp(sw * 0.16f, 180.0f, 206.0f);
    l.toolbar = {0, 0, sw, l.toolbarH};

    const float gap = 6.0f;
    float x = 10.0f;
    const float commandW = sw < 900.0f ? 54.0f : 66.0f;
    l.save = {x, 7, commandW, 28}; x += commandW + gap;
    l.fileName = {x, 7, sw < 900.0f ? 104.0f : 142.0f, 28}; x += l.fileName.width + gap;
    l.load = {x, 7, commandW, 28}; x += commandW + gap;
    l.test = {x, 7, commandW, 28}; x += commandW + gap;
    l.clear = {x, 7, commandW, 28};
    l.exit = {sw - commandW - 10.0f, 7, commandW, 28};

    // Reserve a fixed right-side lane for both W/H controls. The previous
    // compact lane made the H label overlap the W plus button.
    const float sizeAreaW = 232.0f;
    const float toolGap = 5.0f;
    const float toolStart = 10.0f;
    const float toolAvailable = sw - sizeAreaW - toolStart - 12.0f;
    const float toolW = std::clamp((toolAvailable - toolGap * 5.0f) / 6.0f, 48.0f, 78.0f);
    for (int i = 0; i < 6; ++i) {
        l.tools[i] = {toolStart + i * (toolW + toolGap), 44, toolW, 30};
    }
    const float sizeX = sw - sizeAreaW;
    const float stepW = 24.0f;
    l.widthMinus = {sizeX + 54, 44, stepW, 30};
    l.widthPlus = {sizeX + 82, 44, stepW, 30};
    l.heightMinus = {sizeX + 170, 44, stepW, 30};
    l.heightPlus = {sizeX + 198, 44, stepW, 30};

    const float panelY = l.toolbarH + 4.0f;
    const float panelH = std::max(80.0f, sh - panelY - 4.0f);
    l.palette = {2, panelY, l.leftW - 4.0f, panelH};
    const float rightX = sw - l.rightW + 2.0f;
    const float rightPanelW = l.rightW - 4.0f;
    const float layersH = std::min(188.0f, std::max(116.0f, panelH * 0.28f));
    const float minimapH = std::min(174.0f, std::max(105.0f, panelH * 0.27f));
    l.layers = {rightX, panelY, rightPanelW, layersH};
    l.minimap = {rightX, sh - minimapH - 2.0f, rightPanelW, minimapH};
    const float propY = l.layers.y + l.layers.height + 4.0f;
    l.properties = {rightX, propY, rightPanelW,
                    std::max(34.0f, l.minimap.y - propY - 4.0f)};
    l.shortcut = {l.leftW + 10.0f, sh - 30.0f,
                  std::max(120.0f, sw - l.leftW - l.rightW - 20.0f), 24.0f};
    return l;
}

int PaletteColumns(const BuilderLayout& l, float cellWidth) {
    return std::max(1, static_cast<int>((l.palette.width - 16.0f) / cellWidth));
}

Rectangle FirstAnimationFrame(const std::shared_ptr<Animations::TextureAtlas>& atlas,
                               const char* clipName,
                               const Texture2D& fallbackTexture) {
    if (atlas) {
        auto clip = atlas->GetClip(clipName);
        if (clip && !clip->frames.empty()) {
            const Rectangle src = clip->frames.front().src;
            if (src.width > 0.0f && src.height > 0.0f) return src;
        }
    }

    return {0.0f, 0.0f,
            static_cast<float>(fallbackTexture.width),
            static_cast<float>(fallbackTexture.height)};
}

} // namespace

MapBuilderView& MapBuilderView::GetInstance() {
    static MapBuilderView instance;
    return instance;
}

void MapBuilderView::Init() {
    m_showGrid = true;
    m_snapToGrid = true;
    m_currentTool = BuilderTool::Brush;
    m_currentTab = PaletteTab::Tiles;
    m_currentLayer = MapLayer::Main;
    m_selectedEntity = nullptr;
    m_selectedEntityId = 0;
    m_statusMessage.clear();
    m_statusTimer = 0.0f;

    if (!m_resourcesLoaded) {
        struct IconSpec {
            Texture2D* texture;
            const char* atlasPath;
            const char* fallbackPath;
        };
        IconSpec icons[] = {
            {&m_texPlayer, "assets/textures/player/knight_v2/idle_v2.json", "assets/textures/player/knight_v2/idle_v2.png"},
            {&m_texEnemy, "assets/textures/player/ninja_v2/idle_v2.json", "assets/textures/player/ninja_v2/idle_v2.png"},
            {&m_texBoss1, "assets/textures/boss/boss1/phase1/idle.json", "assets/textures/boss/boss1/phase1/idle.png"},
            {&m_texBoss2, "assets/textures/boss/boss2/phase1/idle.json", "assets/textures/boss/boss2/phase1/idle.png"},
            {&m_texBoss3, "assets/textures/boss/boss3/phase1/idle.json", "assets/textures/boss/boss3/phase1/idle.png"},
            {&m_texCoin, "assets/textures/items/coin.json", "assets/textures/items/coin.png"},
            {&m_texPotion, "assets/textures/items/potion_red.json", "assets/textures/items/potion_red.png"},
            {&m_texChest, "assets/textures/objects/chest_closed.json", "assets/textures/objects/chest_closed.png"},
            {&m_texCheckpoint, "assets/textures/objects/checkpoint_uncaptured.json", "assets/textures/objects/checkpoint_uncaptured.png"},
            {&m_texPortalBlue, "assets/textures/objects/portal_blue_anim.json", "assets/textures/objects/portal_blue_spritesheet.png"},
            {&m_texPortalBrown, "assets/textures/objects/portal_brown_anim.json", "assets/textures/objects/portal_brown_spritesheet.png"},
            {&m_texPortalGreen, "assets/textures/objects/portal_green_anim.json", "assets/textures/objects/portal_green_spritesheet.png"},
            {&m_texPortalPurple, "assets/textures/objects/portal_purple_anim.json", "assets/textures/objects/portal_purple_spritesheet.png"},
            {&m_texPortalRed, "assets/textures/objects/portal_red_anim.json", "assets/textures/objects/portal_red_spritesheet.png"}
        };
        static_assert(std::size(icons) == 14);

        for (std::size_t i = 0; i < std::size(icons); ++i) {
            auto atlas = AssetManager::GetInstance().GetAtlas(icons[i].atlasPath);
            if (atlas && atlas->IsTextureLoaded() && atlas->GetTexture()) {
                *icons[i].texture = *atlas->GetTexture();
            } else {
                *icons[i].texture = LoadTexture(icons[i].fallbackPath);
                m_ownsIconTextures[i] = icons[i].texture->id != 0;
            }
            m_iconAtlases[i] = std::move(atlas);
        }

        m_srcPlayer = FirstAnimationFrame(m_iconAtlases[0], "idle", m_texPlayer);
        m_srcEnemy = FirstAnimationFrame(m_iconAtlases[1], "idle", m_texEnemy);
        m_srcBoss1 = FirstAnimationFrame(m_iconAtlases[2], "idle", m_texBoss1);
        m_srcBoss2 = FirstAnimationFrame(m_iconAtlases[3], "idle", m_texBoss2);
        m_srcBoss3 = FirstAnimationFrame(m_iconAtlases[4], "idle", m_texBoss3);
        m_uiFont = LoadFont("assets/fonts/game_font.ttf");
        m_resourcesLoaded = true;
    }
}

void MapBuilderView::Shutdown() {
    if (!m_resourcesLoaded) return;

    Texture2D* textures[] = {
        &m_texPlayer, &m_texEnemy, &m_texBoss1, &m_texBoss2, &m_texBoss3,
        &m_texCoin, &m_texPotion, &m_texChest, &m_texCheckpoint,
        &m_texPortalBlue, &m_texPortalBrown, &m_texPortalGreen,
        &m_texPortalPurple, &m_texPortalRed
    };
    for (std::size_t i = 0; i < std::size(textures); ++i) {
        Texture2D* texture = textures[i];
        if (m_ownsIconTextures[i] && texture->id != 0) ::UnloadTexture(*texture);
        *texture = {};
        m_ownsIconTextures[i] = false;
        m_iconAtlases[i].reset();
    }
    if (m_uiFont.texture.id != 0) {
        ::UnloadFont(m_uiFont);
        m_uiFont = {};
    }
    m_srcPlayer = {};
    m_srcEnemy = {};
    m_srcBoss1 = {};
    m_srcBoss2 = {};
    m_srcBoss3 = {};

    m_selectedEntity = nullptr;
    m_selectedEntityId = 0;
    m_resourcesLoaded = false;
}

bool MapBuilderView::IsMouseOverUI() const {
    if (m_showSaveConfirm) return true; // Save dialog is a full-screen modal

    Vector2 mousePos = GetMousePosition();
    float sw = (float)Renderer::GetInstance().GetWindowWidth();
    float sh = (float)Renderer::GetInstance().GetWindowHeight();

    const BuilderLayout l = BuildBuilderLayout(sw, sh);
    if (CheckCollisionPointRec(mousePos, l.toolbar)) return true;
    if (CheckCollisionPointRec(mousePos, l.palette)) return true;
    if (CheckCollisionPointRec(mousePos, l.layers)) return true;
    if (m_showMinimap && CheckCollisionPointRec(mousePos, l.minimap)) return true;
    if (m_selectedEntity && CheckCollisionPointRec(mousePos, l.properties)) return true;

    return false;
}

void MapBuilderView::Update(float dt) {
    if (m_statusTimer > 0.0f) {
        m_statusTimer = std::max(0.0f, m_statusTimer - dt);
    }

    if (!IsMouseOverUI()) return;

    Vector2 mousePos = GetMousePosition();
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();

    const BuilderLayout layout = BuildBuilderLayout(sw, sh);
    const float tabY = layout.palette.y + 7.0f;
    const float contentY = layout.palette.y + 42.0f;
    const float gridY = contentY + 34.0f;

    if (CheckCollisionPointRec(mousePos, layout.palette) && mousePos.y > gridY && m_currentTab == PaletteTab::Tiles) {
        float wheel = GetMouseWheelMove();
        if (wheel > 0.0f && m_paletteScroll > 0) {
            m_paletteScroll--;
        } else if (wheel < 0.0f) {
            m_paletteScroll++;
        }
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        SoundManager::GetInstance().PlaySound("ui_confirm");
        if (m_showSaveConfirm) {
            float sw = (float)GetScreenWidth();
            float sh = (float)GetScreenHeight();
            Rectangle yesBtn = { sw/2 - 130, sh/2 + 10, 110, 40 };
            Rectangle cancelBtn = { sw/2 + 20, sh/2 + 10, 110, 40 };
            
            if (CheckCollisionPointRec(mousePos, yesBtn)) {
                m_wantsSave = true;
                m_showSaveConfirm = false;
            } else if (CheckCollisionPointRec(mousePos, cancelBtn)) {
                m_showSaveConfirm = false;
            }
            
            // If we click anywhere else, we do NOT close the dialog immediately, 
            // because they might accidentally click the background.
            // Actually, for a modal, usually clicking outside closes it, or does nothing.
            // Let's do nothing if they click the background of the dialog.
            return;
        }

        m_isTypingFileName = CheckCollisionPointRec(mousePos, layout.fileName);

        // Toolbar Clicks
        if (CheckCollisionPointRec(mousePos, layout.toolbar)) {
            if (CheckCollisionPointRec(mousePos, layout.save)) m_showSaveConfirm = true;
            else if (CheckCollisionPointRec(mousePos, layout.load)) m_wantsLoad = true;
            else if (CheckCollisionPointRec(mousePos, layout.test)) m_wantsPlaytest = true;
            else if (CheckCollisionPointRec(mousePos, layout.exit)) m_wantsExit = true;
            else if (CheckCollisionPointRec(mousePos, layout.clear)) m_wantsClearAll = true;

            const BuilderTool toolValues[] = {BuilderTool::Brush, BuilderTool::Eraser,
                BuilderTool::BucketFill, BuilderTool::Select, BuilderTool::BoxSelect,
                BuilderTool::MoveCamera};
            for (int i = 0; i < 6; ++i) {
                if (!CheckCollisionPointRec(mousePos, layout.tools[i])) continue;
                if (i == 5) m_showGrid = !m_showGrid;
                else m_currentTool = toolValues[i];
            }
            if (CheckCollisionPointRec(mousePos, layout.widthMinus)) m_wantsResizeW = -1;
            if (CheckCollisionPointRec(mousePos, layout.widthPlus)) m_wantsResizeW = 1;
            if (CheckCollisionPointRec(mousePos, layout.heightMinus)) m_wantsResizeH = -1;
            if (CheckCollisionPointRec(mousePos, layout.heightPlus)) m_wantsResizeH = 1;
        }
        
        // Layers Panel Clicks
        if (CheckCollisionPointRec(mousePos, layout.layers)) {
            float ly = layout.layers.y + 39.0f;
            for (int i = 0; i < 3; ++i) {
                if (CheckCollisionPointRec(mousePos, {layout.layers.x + 8, ly, layout.layers.width - 54, 25})) {
                    m_currentLayer = static_cast<MapLayer>(i);
                }
                if (CheckCollisionPointRec(mousePos, {layout.layers.x + layout.layers.width - 39, ly, 30, 25})) {
                    m_layerVisible[i] = !m_layerVisible[i];
                }
                ly += 30;
            }
        }
        
        // Palette Clicks (Left Panel)
        if (CheckCollisionPointRec(mousePos, layout.palette)) {
            // Tabs
            const float tabW = (layout.palette.width - 20.0f) / 3.0f;
            if (mousePos.y >= tabY && mousePos.y < tabY + 25.0f) {
                for (int i = 0; i < 3; ++i) {
                    if (CheckCollisionPointRec(mousePos, {layout.palette.x + 7 + i * (tabW + 3), tabY, tabW, 25}))
                        m_currentTab = static_cast<PaletteTab>(i);
                }
            }
            
            if (m_currentTab == PaletteTab::Tiles) {
                // Tileset switcher
                if (mousePos.y >= contentY && mousePos.y < gridY) {
                    if (CheckCollisionPointRec(mousePos, {layout.palette.x + layout.palette.width - 58, contentY + 4, 22, 24})) {
                        m_selectedTileType--;
                        if (m_selectedTileType < 1) m_selectedTileType = 6;
                        m_selectedTileId = 0;
                        m_paletteScroll = 0;
                    }
                    if (CheckCollisionPointRec(mousePos, {layout.palette.x + layout.palette.width - 31, contentY + 4, 22, 24})) {
                        m_selectedTileType++;
                        if (m_selectedTileType > 6) m_selectedTileType = 1;
                        m_selectedTileId = 0;
                        m_paletteScroll = 0;
                    }
                }
                // Tile selection
                if (mousePos.y >= gridY) {
                    int col = ((int)mousePos.x - (int)(layout.palette.x + 8)) / 40;
                    int row = ((int)mousePos.y - (int)gridY) / 40;
                    int cols = PaletteColumns(layout, 40.0f);
                    if (col >= 0 && col < cols && row >= 0) {
                        int idx = m_paletteScroll * cols + row * cols + col;
                        m_selectedTileId = idx + 1;
                    }
                }
            } else if (mousePos.y >= contentY) {
                const int cols = PaletteColumns(layout, 55.0f);
                const int col = ((int)mousePos.x - (int)(layout.palette.x + 8)) / 55;
                const int row = ((int)mousePos.y - (int)(contentY + 4)) / 55;
                const int idx = row * cols + col;
                if (m_currentTab == PaletteTab::Entities) {
                    const EntityType types[] = {EntityType::Player, EntityType::Enemy, EntityType::Enemy,
                        EntityType::Boss, EntityType::Boss, EntityType::Boss, EntityType::Item,
                        EntityType::Item};
                    const int subs[] = {0,0,1,1,2,3,0,3};
                    if (idx >= 0 && idx < 8) { m_selectedEntityType = types[idx]; m_selectedEntitySubType = subs[idx]; }
                } else if (m_currentTab == PaletteTab::Triggers) {
                    const EntityType types[] = {EntityType::Chest, EntityType::Checkpoint,
                        EntityType::TeleportPortal, EntityType::TeleportPortal, EntityType::TeleportPortal,
                        EntityType::TeleportPortal, EntityType::TeleportPortal, EntityType::TeleportPortal,
                        EntityType::TeleportPortal, EntityType::TeleportPortal, EntityType::TeleportPortal,
                        EntityType::TeleportPortal};
                    const int subs[] = {0,0,100,101,102,103,104,200,201,202,203,204};
                    if (idx >= 0 && idx < 12) { m_selectedEntityType = types[idx]; m_selectedEntitySubType = subs[idx]; }
                }
            }
        }
    }
}

void MapBuilderView::DrawEditorText(const char* text, Vector2 pos, float size, Color color) const {
    const Font font = (m_uiFont.texture.id != 0) ? m_uiFont : GetFontDefault();
    DrawTextEx(font, text, pos, size, 1.0f, color);
}

void MapBuilderView::DrawEditorPanel(Rectangle rect, const char* title) const {
    const Color panel = {20, 16, 38, 244};
    const Color panelTop = {43, 31, 67, 246};
    const Color gold = {226, 178, 78, 255};
    DrawRectangleRounded({rect.x + 4, rect.y + 5, rect.width, rect.height}, 0.055f, 8,
                         Fade(BLACK, 0.42f));
    DrawRectangleRounded(rect, 0.055f, 8, panel);
    DrawRectangleGradientV((int)rect.x + 2, (int)rect.y + 2,
                           (int)rect.width - 4, std::min(42, (int)rect.height - 4),
                           panelTop, panel);
    DrawRectangleRoundedLinesEx(rect, 0.055f, 8, 2.0f, Fade(gold, 0.82f));
    if (title) {
        DrawEditorText(title, {rect.x + 12, rect.y + 8}, 17.0f, gold);
        DrawLineEx({rect.x + 10, rect.y + 31}, {rect.x + rect.width - 10, rect.y + 31},
                   1.0f, Fade(gold, 0.42f));
    }
}

void MapBuilderView::DrawEditorButton(Rectangle rect, const char* label, bool active,
                                      Color accent) const {
    const bool hovered = CheckCollisionPointRec(GetMousePosition(), rect);
    Color fill = active ? Color{78, 51, 101, 255} : Color{31, 26, 52, 245};
    if (hovered) fill = active ? Color{101, 67, 128, 255} : Color{50, 39, 75, 250};
    DrawRectangleRounded({rect.x + 2, rect.y + 3, rect.width, rect.height}, 0.18f, 6,
                         Fade(BLACK, 0.45f));
    DrawRectangleRounded(rect, 0.18f, 6, fill);
    DrawRectangleRoundedLinesEx(rect, 0.18f, 6, active ? 2.0f : 1.0f,
                                active ? accent : Fade(accent, hovered ? 0.85f : 0.45f));
    const Font font = (m_uiFont.texture.id != 0) ? m_uiFont : GetFontDefault();
    float fontSize = rect.height >= 30.0f ? 12.0f : 10.0f;
    Vector2 size = MeasureTextEx(font, label, fontSize, 1.0f);
    while (fontSize > 8.0f && size.x > rect.width - 8.0f) {
        fontSize -= 1.0f;
        size = MeasureTextEx(font, label, fontSize, 1.0f);
    }
    DrawTextEx(font, label,
               {rect.x + (rect.width - size.x) * 0.5f, rect.y + (rect.height - size.y) * 0.5f},
               fontSize, 1.0f, active ? RAYWHITE : Fade(RAYWHITE, 0.82f));
}

void MapBuilderView::RenderUI(const Camera2D& camera, GameState* state) {
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();

    DrawToolbar(state, sw, sh);
    DrawPalette(sw, sh);
    DrawLayersPanel(sw, sh);
    if (m_selectedEntity) DrawPropertiesPanel(sw, sh);
    if (m_showMinimap) DrawMinimap(camera, state, sw, sh);

    const BuilderLayout layout = BuildBuilderLayout(sw, sh);
    Rectangle shortcutBar = layout.shortcut;
    DrawRectangleRounded(shortcutBar, 0.3f, 6, Fade(Color{18, 14, 34, 255}, 0.90f));
    DrawRectangleRoundedLinesEx(shortcutBar, 0.3f, 6, 1.0f, Fade(Color{226, 178, 78, 255}, 0.45f));
    const char* shortcuts = shortcutBar.width > 500.0f
        ? "LMB  PAINT     RMB  PAN     WHEEL  ZOOM     CTRL+Z  UNDO"
        : "LMB PAINT   RMB PAN   WHEEL ZOOM";
    const Font uiFont = (m_uiFont.texture.id != 0) ? m_uiFont : GetFontDefault();
    Vector2 shortcutSize = MeasureTextEx(uiFont, shortcuts, 10.0f, 1.0f);
    DrawEditorText(shortcuts,
                   {shortcutBar.x + (shortcutBar.width - shortcutSize.x) * 0.5f, shortcutBar.y + 7.0f},
                   10.0f, Fade(RAYWHITE, 0.70f));

    if (m_statusTimer > 0.0f && !m_statusMessage.empty()) {
        const float alpha = std::min(1.0f, m_statusTimer * 2.0f);
        Vector2 textSize = MeasureTextEx(uiFont, m_statusMessage.c_str(), 14.0f, 1.0f);
        Rectangle toast = {sw * 0.5f - textSize.x * 0.5f - 22.0f, layout.toolbarH + 10.0f,
                           textSize.x + 44.0f, 38.0f};
        DrawRectangleRounded(toast, 0.25f, 8, Fade(Color{25, 61, 54, 255}, alpha * 0.96f));
        DrawRectangleRoundedLinesEx(toast, 0.25f, 8, 2.0f,
                                    Fade(Color{105, 235, 169, 255}, alpha));
        DrawEditorText(m_statusMessage.c_str(),
                       {toast.x + 22.0f, toast.y + 11.0f}, 14.0f,
                       Fade(RAYWHITE, alpha));
    }
    if (m_isTypingFileName) {
        int key = GetCharPressed();
        while (key > 0) {
            if ((key >= 32) && (key <= 125) && m_fileName.length() < 20) m_fileName += (char)key;
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && m_fileName.length() > 0) {
            m_fileName.pop_back();
        }
    }
    
    if (m_showSaveConfirm) {
        DrawRectangle(0, 0, (int)sw, (int)sh, Fade(BLACK, 0.76f));
        Rectangle box = {sw/2 - 190, sh/2 - 105, 380, 210};
        DrawEditorPanel(box, "SAVE CUSTOM MAP");
        DrawEditorText("This becomes the playable custom map", {box.x + 30, box.y + 54},
                       13.0f, Fade(RAYWHITE, 0.72f));
        std::string msg = m_fileName + ".lvl";
        Rectangle namePlate = {box.x + 30, box.y + 82, box.width - 60, 42};
        DrawRectangleRounded(namePlate, 0.18f, 6, Color{13, 12, 25, 255});
        DrawRectangleRoundedLinesEx(namePlate, 0.18f, 6, 1.0f, Fade(SKYBLUE, 0.55f));
        Vector2 msgSize = MeasureTextEx(uiFont, msg.c_str(), 16.0f, 1.0f);
        DrawEditorText(msg.c_str(), {namePlate.x + (namePlate.width-msgSize.x)*0.5f, namePlate.y+13},
                       16.0f, RAYWHITE);
        DrawEditorButton({sw/2 - 130, sh/2 + 10, 110, 40}, "SAVE", true,
                         Color{105, 235, 169, 255});
        DrawEditorButton({sw/2 + 20, sh/2 + 10, 110, 40}, "CANCEL", false,
                         Color{235, 105, 118, 255});
    }
}


void MapBuilderView::DrawToolbar(GameState* state, int sw, int sh) {
    const Color gold = {226, 178, 78, 255};
    const Color mint = {105, 235, 169, 255};
    const Color coral = {235, 105, 118, 255};
    const BuilderLayout l = BuildBuilderLayout((float)sw, (float)sh);
    DrawRectangleGradientV(0, 0, sw, (int)l.toolbarH, Color{48, 34, 71, 255}, Color{20, 16, 38, 255});
    DrawRectangle(0, (int)l.toolbarH - 2, sw, 2, Fade(gold, 0.75f));
    DrawLine(8, 40, sw - 8, 40, Fade(gold, 0.22f));

    DrawEditorButton(l.save, "SAVE", false, mint);
    
    Color boxCol = m_isTypingFileName ? gold : Fade(SKYBLUE, 0.72f);
    DrawRectangleRounded(l.fileName, 0.16f, 6, Color{13, 12, 25, 255});
    DrawRectangleRoundedLinesEx(l.fileName, 0.16f, 6,
                                m_isTypingFileName ? 2.0f : 1.0f, boxCol);
    std::string visibleName = m_fileName;
    const Font font = (m_uiFont.texture.id != 0) ? m_uiFont : GetFontDefault();
    while (visibleName.size() > 4 && MeasureTextEx(font, visibleName.c_str(), 11.0f, 1.0f).x > l.fileName.width - 14.0f)
        visibleName.erase(visibleName.begin());
    DrawEditorText(visibleName.c_str(), {l.fileName.x + 7, l.fileName.y + 9}, 11.0f, RAYWHITE);
    if (m_isTypingFileName && ((int)(GetTime() * 2) % 2 == 0)) {
        float textW = MeasureTextEx(font, visibleName.c_str(), 11.0f, 1.0f).x;
        DrawLineEx({l.fileName.x + 7 + textW + 2, l.fileName.y + 6},
                   {l.fileName.x + 7 + textW + 2, l.fileName.y + 22}, 1.0f, boxCol);
    }
    DrawEditorButton(l.load, "LOAD", false, SKYBLUE);
    DrawEditorButton(l.test, "TEST", false, mint);
    DrawEditorButton(l.clear, "CLEAR", false, ORANGE);
    DrawEditorButton(l.exit, "EXIT", false, coral);

    const char* toolNames[] = {"BRUSH", "ERASE", "FILL", "SELECT", "BOX", "GRID"};
    const bool active[] = {m_currentTool == BuilderTool::Brush, m_currentTool == BuilderTool::Eraser,
        m_currentTool == BuilderTool::BucketFill, m_currentTool == BuilderTool::Select,
        m_currentTool == BuilderTool::BoxSelect, m_showGrid};
    for (int i = 0; i < 6; ++i) DrawEditorButton(l.tools[i], toolNames[i], active[i], i == 5 ? mint : gold);

    if (state) {
        DrawEditorText(TextFormat("W %d", state->GetMapWidth()), {l.widthMinus.x - 48, 54}, 10.0f, RAYWHITE);
        DrawEditorButton(l.widthMinus, "-", false, SKYBLUE);
        DrawEditorButton(l.widthPlus, "+", false, SKYBLUE);
        DrawEditorText(TextFormat("H %d", state->GetMapHeight()), {l.heightMinus.x - 48, 54}, 10.0f, RAYWHITE);
        DrawEditorButton(l.heightMinus, "-", false, SKYBLUE);
        DrawEditorButton(l.heightPlus, "+", false, SKYBLUE);
    }
}

void MapBuilderView::DrawPalette(int sw, int sh) {
    const Color gold = {226, 178, 78, 255};
    const BuilderLayout l = BuildBuilderLayout((float)sw, (float)sh);
    DrawEditorPanel(l.palette);
    const float tabY = l.palette.y + 7.0f;
    const float tabW = (l.palette.width - 20.0f) / 3.0f;
    const float contentY = l.palette.y + 42.0f;
    const float gridY = contentY + 34.0f;
    
    // Tabs
    const char* tabs[] = {"TILES", "ACTORS", "OBJECTS"};
    for (int i = 0; i < 3; ++i)
        DrawEditorButton({l.palette.x + 7 + i * (tabW + 3), tabY, tabW, 25}, tabs[i],
                         (int)m_currentTab == i, gold);
    DrawLineEx({l.palette.x + 8, contentY - 4}, {l.palette.x + l.palette.width - 8, contentY - 4}, 1.0f, Fade(gold, 0.35f));

    // Items
    if (m_currentTab == PaletteTab::Tiles) {
        // Draw tileset selector
        DrawEditorText(TextFormat("TILESET  %d", m_selectedTileType), {l.palette.x + 10, contentY + 10}, 12.0f, RAYWHITE);
        DrawEditorButton({l.palette.x + l.palette.width - 58, contentY + 4, 22, 24}, "<", false, SKYBLUE);
        DrawEditorButton({l.palette.x + l.palette.width - 31, contentY + 4, 22, 24}, ">", false, SKYBLUE);
        DrawLineEx({l.palette.x + 8, gridY - 3}, {l.palette.x + l.palette.width - 8, gridY - 3}, 1.0f, Fade(gold, 0.28f));

        auto ts = View::GameView::GetInstance().GetTileset(m_selectedTileType);
        if (ts && ts->texture.id != 0) {
            int tileW = ts->texture.width / ts->gridCols;
            int tileH = tileW;
            if (tileH == 0) tileH = 32;

            int totalTiles = (ts->texture.width / tileW) * (ts->texture.height / tileH);
            int cols = PaletteColumns(l, 40.0f);
            int startIdx = m_paletteScroll * cols;
            int drawn = 0;
            
            int maxRows = std::max(1, (int)((l.palette.y + l.palette.height - gridY - 5) / 40));
            for (int i = startIdx; i < totalTiles && drawn < cols * maxRows; ++i) {
                int id = i + 1;
                int row = drawn / cols;
                int col = drawn % cols;
                int dx = (int)l.palette.x + 8 + col * 40;
                int dy = (int)gridY + row * 40;

                bool selected = (m_selectedTileId == id);
                DrawRectangleRounded({(float)dx, (float)dy, 36, 36}, 0.15f, 5,
                                     selected ? Color{83, 57, 106, 255} : Color{28, 24, 46, 245});
                DrawRectangleRoundedLinesEx({(float)dx, (float)dy, 36, 36}, 0.15f, 5,
                                            selected ? 2.0f : 1.0f,
                                            selected ? gold : Fade(SKYBLUE, 0.32f));

                int texX = (i % ts->gridCols) * tileW;
                int texY = (i / ts->gridCols) * tileH;
                Rectangle src = { (float)texX, (float)texY, (float)tileW, (float)tileH };
                Rectangle dest = { (float)dx + 2, (float)dy + 2, 32, 32 };
                DrawTexturePro(ts->texture, src, dest, {0,0}, 0.0f, WHITE);
                
                drawn++;
            }
        }
    } else if (m_currentTab == PaletteTab::Entities) {
        const char* names[] = {"Player", "Melee", "Ranged", "Boss 1", "Boss 2", "Boss 3", "Coin", "Potion"};
        EntityType types[] = {EntityType::Player, EntityType::Enemy, EntityType::Enemy, EntityType::Boss, EntityType::Boss, EntityType::Boss, EntityType::Item, EntityType::Item};
        int subTypes[] = {0, 0, 1, 1, 2, 3, 0, 3}; // Apple is not exposed in the editor palette.
        Texture2D texs[] = {m_texPlayer, m_texEnemy, m_texEnemy, m_texBoss1, m_texBoss2, m_texBoss3, m_texCoin, m_texPotion};
        Rectangle sources[] = {
            m_srcPlayer, m_srcEnemy, m_srcEnemy,
            m_srcBoss1, m_srcBoss2, m_srcBoss3,
            {0, 0, static_cast<float>(m_texCoin.width), static_cast<float>(m_texCoin.height)},
            {0, 0, static_cast<float>(m_texPotion.width), static_cast<float>(m_texPotion.height)}
        };
        
        const int cols = PaletteColumns(l, 55.0f);
        for (int i = 0; i < 8; ++i) {
            int col = i % cols;
            int row = i / cols;
            float px = l.palette.x + 8 + col * 55;
            float py = contentY + 4 + row * 55;

            bool isSelected = false;
            if (types[i] == EntityType::Player) {
                isSelected = (m_selectedEntityType == EntityType::Player);
            } else {
                isSelected = (m_selectedEntityType == types[i] && m_selectedEntitySubType == subTypes[i]);
            }

            DrawRectangleRounded({px, py, 50, 45}, 0.14f, 5,
                                 isSelected ? Color{83, 57, 106, 255} : Color{28, 24, 46, 245});
            DrawRectangleRoundedLinesEx({px, py, 50, 45}, 0.14f, 5,
                                        isSelected ? 2.0f : 1.0f,
                                        isSelected ? gold : Fade(SKYBLUE, 0.32f));
            if (texs[i].id != 0) {
                const Rectangle src = sources[i];
                Rectangle dest = { px + 9.0f, py + 2.0f, 32.0f, 32.0f };
                DrawTexturePro(texs[i], src, dest, {0,0}, 0.0f, WHITE);
            }
            DrawEditorText(names[i], {px + 4, py + 34}, 8.0f, RAYWHITE);
        }
    } else if (m_currentTab == PaletteTab::Triggers) {
        // Types: Chest, Checkpoint, and 10 Portals (5 Local, 5 LevelTrans)
        const char* names[] = {
            "Chest", "Check",
            "L_Blue", "L_Brown", "L_Green", "L_Purple", "L_Red",
            "T_Blue", "T_Brown", "T_Green", "T_Purple", "T_Red"
        };
        EntityType types[] = {
            EntityType::Chest, EntityType::Checkpoint,
            EntityType::TeleportPortal, EntityType::TeleportPortal, EntityType::TeleportPortal, EntityType::TeleportPortal, EntityType::TeleportPortal,
            EntityType::TeleportPortal, EntityType::TeleportPortal, EntityType::TeleportPortal, EntityType::TeleportPortal, EntityType::TeleportPortal
        };
        // subTypes mapping: 100+c for Local, 200+c for Transition
        int subTypes[] = {
            0, 0,
            100, 101, 102, 103, 104,
            200, 201, 202, 203, 204
        };
        Texture2D texs[] = {
            m_texChest, m_texCheckpoint,
            m_texPortalBlue, m_texPortalBrown, m_texPortalGreen, m_texPortalPurple, m_texPortalRed,
            m_texPortalBlue, m_texPortalBrown, m_texPortalGreen, m_texPortalPurple, m_texPortalRed
        };

        const int cols = PaletteColumns(l, 55.0f);
        for (int i = 0; i < 12; ++i) {
            int col = i % cols;
            int row = i / cols;
            float px = l.palette.x + 8 + col * 55;
            float py = contentY + 4 + row * 55;
            
            bool isSelected = false;
            if (types[i] == EntityType::Chest || types[i] == EntityType::Checkpoint) {
                isSelected = (m_selectedEntityType == types[i]);
            } else {
                isSelected = (m_selectedEntityType == types[i] && m_selectedEntitySubType == subTypes[i]);
            }

            DrawRectangleRounded({px, py, 50, 45}, 0.14f, 5,
                                 isSelected ? Color{83, 57, 106, 255} : Color{28, 24, 46, 245});
            DrawRectangleRoundedLinesEx({px, py, 50, 45}, 0.14f, 5,
                                        isSelected ? 2.0f : 1.0f,
                                        isSelected ? gold : Fade(SKYBLUE, 0.32f));
            if (texs[i].id != 0) {
                float frameW = (float)texs[i].height;
                if (types[i] == EntityType::Chest) frameW = (float)texs[i].width;
                else if (types[i] == EntityType::TeleportPortal) frameW = 283; // from json
                Rectangle src = { 0, 0, frameW, (float)texs[i].height };
                Rectangle dest = { px + 9.0f, py + 2.0f, 32.0f, 32.0f };
                DrawTexturePro(texs[i], src, dest, {0,0}, 0.0f, WHITE);
            }
            DrawEditorText(names[i], {px + 2, py + 34}, 7.0f, RAYWHITE);
        }
    }
}

void MapBuilderView::DrawLayersPanel(int sw, int sh) {
    const Color gold = {226, 178, 78, 255};
    const BuilderLayout l = BuildBuilderLayout((float)sw, (float)sh);
    DrawEditorPanel(l.layers, "LAYERS");

    const char* layerNames[] = {"Background", "Main", "Foreground"};
    float y = l.layers.y + 39.0f;
    for (int i = 0; i < 3; ++i) {
        const bool selected = m_currentLayer == static_cast<MapLayer>(i);
        Rectangle row = {l.layers.x + 8, y, l.layers.width - 54, 25};
        DrawRectangleRounded(row, 0.2f, 5,
                             selected ? Color{76, 53, 98, 255} : Color{30, 25, 49, 225});
        DrawRectangleRoundedLinesEx(row, 0.2f, 5, selected ? 1.5f : 1.0f,
                                    selected ? gold : Fade(SKYBLUE, 0.25f));
        DrawEditorText(layerNames[i], {row.x + 8, y + 7}, 10.0f,
                       selected ? RAYWHITE : Fade(RAYWHITE, 0.72f));

        Color vColor = m_layerVisible[i] ? Color{105, 235, 169, 255} : Color{235, 105, 118, 255};
        DrawEditorButton({l.layers.x + l.layers.width - 39, y, 30, 25}, m_layerVisible[i] ? "ON" : "OFF",
                         m_layerVisible[i], vColor);
        
        y += 35;
    }

    if (l.layers.height >= 170.0f)
        DrawEditorText("Choose the active paint layer", {l.layers.x + 10, l.layers.y + l.layers.height - 20}, 8.0f,
                       Fade(RAYWHITE, 0.48f));
}

void MapBuilderView::DrawPropertiesPanel(int sw, int sh) {
    const Color gold = {226, 178, 78, 255};
    const BuilderLayout l = BuildBuilderLayout((float)sw, (float)sh);
    DrawEditorPanel(l.properties, "PROPERTIES");
    
    if (m_selectedEntity) {
        float y = l.properties.y + 40;
        const float textX = l.properties.x + 10;
        DrawEditorText(TextFormat("ID   %d", m_selectedEntity->GetId()), {textX, y}, 11.0f, RAYWHITE); y += 24;
        DrawEditorText(TextFormat("POS  %.0f, %.0f", m_selectedEntity->GetPosition().x, m_selectedEntity->GetPosition().y), {textX, y}, 11.0f, RAYWHITE); y += 24;
        DrawLineEx({textX, y}, {l.properties.x + l.properties.width - 10, y}, 1.0f, Fade(gold, 0.25f)); y += 14;
        
        if (auto* tz = dynamic_cast<TriggerZone*>(m_selectedEntity)) {
            DrawEditorText("TRIGGER ZONE", {textX, y}, 11.0f, Color{105, 235, 169, 255}); y += 24;
            DrawEditorText(TextFormat("TARGET  %s", tz->GetTargetLevelId().c_str()), {textX, y}, 9.0f, RAYWHITE); y += 24;
            DrawEditorButton({textX, y, std::min(112.0f, l.properties.width - 20), 28}, "EDIT TARGET", false, SKYBLUE);
        } else if (auto* chest = dynamic_cast<Chest*>(m_selectedEntity)) {
            (void)chest;
            DrawEditorText("CHEST", {textX, y}, 11.0f, ORANGE); y += 25;
        } else if (auto* boss = dynamic_cast<Boss*>(m_selectedEntity)) {
            (void)boss;
            DrawEditorText("BOSS", {textX, y}, 11.0f, Color{235, 105, 118, 255}); y += 25;
        } else {
            DrawEditorText(TextFormat("TYPE  %d", (int)m_selectedEntity->GetType()), {textX, y}, 11.0f, LIGHTGRAY); y += 25;
        }
    } else {
        DrawEditorText("NO ENTITY SELECTED", {l.properties.x + 10, l.properties.y + 42}, 10.0f, GRAY);
    }
}

void MapBuilderView::DrawMinimap(const Camera2D& camera, GameState* state, int sw, int sh) {
    if (!state) return;
    const Color gold = {226, 178, 78, 255};
    const BuilderLayout l = BuildBuilderLayout((float)sw, (float)sh);
    Rectangle outer = l.minimap;
    DrawEditorPanel(outer, "MINIMAP");
    Rectangle mmBox = {outer.x + 8, outer.y + 38, outer.width - 16, outer.height - 46};
    DrawRectangleRounded(mmBox, 0.05f, 5, Color{11, 12, 24, 255});
    DrawRectangleRoundedLinesEx(mmBox, 0.05f, 5, 1.0f, Fade(SKYBLUE, 0.35f));

    float mapW = state->GetMapWidth() * TILE_SIZE;
    float mapH = state->GetMapHeight() * TILE_SIZE;
    if (mapW == 0 || mapH == 0) return;

    float scaleX = mmBox.width / mapW;
    float scaleY = mmBox.height / mapH;

    for (const auto& tile : state->GetTiles(MapLayer::Main)) {
        if (!tile.solid) continue;
        float tx = mmBox.x + tile.x * TILE_SIZE * scaleX;
        float ty = mmBox.y + tile.y * TILE_SIZE * scaleY;
        DrawRectangle(tx, ty, TILE_SIZE * scaleX, TILE_SIZE * scaleY, GRAY);
    }

    // Draw camera viewport in minimap
    float vw = (sw / camera.zoom) * scaleX;
    float vh = (sh / camera.zoom) * scaleY;
    float vx = mmBox.x + (camera.target.x - (sw/camera.zoom)/2.0f) * scaleX;
    float vy = mmBox.y + (camera.target.y - (sh/camera.zoom)/2.0f) * scaleY;
    DrawRectangleLinesEx({vx, vy, vw, vh}, 2.0f, gold);
}

void MapBuilderView::RenderWorldOverlay(const Camera2D& camera, GameState* state, Vector2 mouseWorldPos) {
    if (m_showGrid && state) {
        float mapW = state->GetMapWidth() * TILE_SIZE;
        float mapH = state->GetMapHeight() * TILE_SIZE;

        // Draw grid lines
        for (float x = 0; x <= mapW; x += TILE_SIZE) {
            DrawLineV({x, 0}, {x, mapH}, Fade(LIGHTGRAY, 0.3f));
        }
        for (float y = 0; y <= mapH; y += TILE_SIZE) {
            DrawLineV({0, y}, {mapW, y}, Fade(LIGHTGRAY, 0.3f));
        }
    }

    // Highlight hovered tile
    if (!IsMouseOverUI()) {
        int tx = (int)(mouseWorldPos.x / TILE_SIZE);
        int ty = (int)(mouseWorldPos.y / TILE_SIZE);
        DrawRectangleLines(tx * TILE_SIZE, ty * TILE_SIZE, TILE_SIZE, TILE_SIZE, YELLOW);
    }

    // Draw Entities from GameState
    if (state) {
        for (const auto& entity : state->GetAllEntities()) {
            if (!entity) continue;
            Vector2 pos = entity->GetPosition();
            EntityType type = entity->GetType();
            
            // Draw a colored bounding box for the entity
            Color ec = MAGENTA;
            if (type == EntityType::Player) ec = BLUE;
            else if (type == EntityType::Enemy || type == EntityType::Boss) ec = RED;
            else if (type == EntityType::Item) ec = YELLOW;
            else if (type == EntityType::Chest) ec = ORANGE;
            else if (type == EntityType::Checkpoint) ec = GREEN;

            DrawRectangleLines(pos.x, pos.y, TILE_SIZE, TILE_SIZE, ec);
            
            // Also draw a label to be clear
            const char* label = "E";
            if (type == EntityType::Player) label = "P";
            else if (type == EntityType::Enemy) label = "E";
            else if (type == EntityType::Boss) label = "B";
            else if (type == EntityType::Item) label = "I";
            else if (type == EntityType::Chest) label = "C";
            else if (type == EntityType::Checkpoint) label = "CP";
            else if (type == EntityType::TriggerZone) label = "TZ";
            else if (type == EntityType::FakeWall) label = "FW";
            else if (type == EntityType::TeleportPortal) label = "PTL";

            DrawText(label, pos.x + 4, pos.y + 4, 10, ec);
        }
    }

    // Draw selection box if active or if we have a selection
    auto& ctrl = MapBuilderController::GetInstance();
    if (ctrl.IsBoxSelecting() || m_currentTool == BuilderTool::BoxSelect) {
        Rectangle selBox = ctrl.GetSelectionBox();
        if (selBox.width != 0 || selBox.height != 0) {
            DrawRectangleLinesEx(selBox, 2.0f, RED);
            DrawRectangleRec(selBox, Fade(RED, 0.2f));
        }
    }
}

} // namespace View
