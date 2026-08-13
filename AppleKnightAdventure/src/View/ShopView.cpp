#include "View/ShopView.h"
#include "View/AssetManager.h"
#include "Systems/SoundManager.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>

using namespace View;

namespace {

float LerpValue(float a, float b, float t) { return a + (b - a) * t; }
float EaseOutCubic(float t) { const float f = 1.0f - t; return 1.0f - f*f*f; }

struct ShopLayout {
    Rectangle panel{};
    Rectangle back{};
    Rectangle coin{};
    Rectangle tabs[2]{};
    Rectangle grid{};
    Rectangle detail{};
    Rectangle action{};
};

ShopLayout BuildShopLayout(float slide) {
    const float sw = static_cast<float>(GetScreenWidth());
    const float sh = static_cast<float>(GetScreenHeight());
    const float panelW = std::min(sw * 0.92f, 1240.0f);
    const float panelH = std::min(sh * 0.86f, 690.0f);
    const float finalY = (sh - panelH) * 0.54f;
    const float y = finalY + (1.0f - EaseOutCubic(slide)) * sh * 0.45f;
    const float x = (sw - panelW) * 0.5f;
    const float headerH = std::clamp(panelH * 0.105f, 42.0f, 68.0f);
    const float footerH = std::clamp(panelH * 0.105f, 42.0f, 70.0f);
    const float pad = std::clamp(panelW * 0.016f, 10.0f, 20.0f);
    const float leftW = panelW * 0.54f;

    ShopLayout l;
    l.panel = {x, y, panelW, panelH};
    l.back = {x + pad, y + 10.0f, std::clamp(panelW * 0.10f, 86.0f, 124.0f), headerH - 18.0f};
    l.coin = {x + panelW - std::clamp(panelW * 0.18f, 132.0f, 210.0f) - pad,
              y + 10.0f, std::clamp(panelW * 0.18f, 132.0f, 210.0f), headerH - 18.0f};
    const float tabY = y + headerH;
    const float tabGap = 8.0f;
    const float tabW = (leftW - pad * 2.0f - tabGap) * 0.5f;
    const float tabH = std::clamp(panelH * 0.075f, 34.0f, 50.0f);
    l.tabs[0] = {x + pad, tabY, tabW, tabH};
    l.tabs[1] = {x + pad + tabW + tabGap, tabY, tabW, tabH};
    l.grid = {x + pad, tabY + tabH + 10.0f,
              leftW - pad * 2.0f, panelH - headerH - tabH - footerH - 16.0f};
    l.detail = {x + leftW + pad * 0.5f, y + headerH,
                panelW - leftW - pad * 1.5f, panelH - headerH - footerH * 0.35f};
    const float actionW = std::min(l.detail.width * 0.68f, 270.0f);
    const float actionH = std::clamp(footerH * 0.72f, 38.0f, 54.0f);
    l.action = {l.detail.x + (l.detail.width - actionW) * 0.5f,
                y + panelH - actionH - 15.0f, actionW, actionH};
    return l;
}

Rectangle GridCard(Rectangle grid, int index) {
    constexpr int columns = 2;
    const float gap = std::clamp(grid.width * 0.025f, 7.0f, 14.0f);
    const float cardW = (grid.width - gap) * 0.5f;
    const float cardH = std::max(92.0f, (grid.height - gap) * 0.5f);
    return {grid.x + (index % columns) * (cardW + gap),
            grid.y + (index / columns) * (cardH + gap), cardW, cardH};
}

std::shared_ptr<Animations::AnimationClip> IdleClip(const std::shared_ptr<Animations::TextureAtlas>& atlas) {
    if (!atlas) return nullptr;
    for (const char* name : {"idle", "Idle", "IDLE", "default"}) {
        if (atlas->HasClip(name)) return atlas->GetClip(name);
    }
    const auto names = atlas->GetClipNames();
    return names.empty() ? nullptr : atlas->GetClip(names.front());
}

void DrawAtlasThumbnail(const std::shared_ptr<Animations::TextureAtlas>& atlas,
                        Rectangle area, Color tint) {
    const auto clip = IdleClip(atlas);
    if (!atlas || !clip || clip->frames.empty() || !atlas->GetTexture()) return;
    const Rectangle src = clip->frames.front().src;
    if (src.width <= 0.0f || src.height <= 0.0f) return;
    const float scale = std::min(area.width / std::fabs(src.width), area.height / std::fabs(src.height));
    const float dw = std::fabs(src.width) * scale;
    const float dh = std::fabs(src.height) * scale;
    DrawTexturePro(*atlas->GetTexture(), src,
                   {area.x + (area.width - dw) * 0.5f, area.y + (area.height - dh) * 0.5f, dw, dh},
                   {0,0}, 0.0f, tint);
}

void DrawWrapped(Font font, const std::string& text, Rectangle bounds,
                 float size, Color color) {
    std::istringstream stream(text);
    std::string word;
    std::string line;
    float y = bounds.y;
    while (stream >> word) {
        const std::string candidate = line.empty() ? word : line + " " + word;
        if (!line.empty() && MeasureTextEx(font, candidate.c_str(), size, 1.0f).x > bounds.width) {
            DrawTextEx(font, line.c_str(), {bounds.x, y}, size, 1.0f, color);
            line = word;
            y += size * 1.42f;
            if (y + size > bounds.y + bounds.height) return;
        } else line = candidate;
    }
    if (!line.empty() && y + size <= bounds.y + bounds.height)
        DrawTextEx(font, line.c_str(), {bounds.x, y}, size, 1.0f, color);
}

} // namespace

ShopView& ShopView::GetInstance() {
    static ShopView instance;
    return instance;
}

bool ShopView::Init() {
    if (!m_fontLoaded && FileExists("assets/fonts/game_font.ttf")) {
        m_font = LoadFont("assets/fonts/game_font.ttf");
        m_fontLoaded = m_font.texture.id != 0;
    }
    m_visible = false;
    m_activeTab = ShopTab::Characters;
    m_selectedIdx = 0;
    m_slideT = 0.0f;
    m_animTime = 0.0f;
    return true;
}

void ShopView::Shutdown() {
    m_previewAtlas.reset();
    m_charThumbnails.clear();
    m_petThumbnails.clear();
    if (m_fontLoaded) {
        UnloadFont(m_font);
        m_font = {};
        m_fontLoaded = false;
    }
    m_visible = false;
}

void ShopView::SetCharacterItems(const std::vector<ShopItemData>& items) {
    m_charItems = items;
    SyncThumbnailAtlases();
}

void ShopView::SetPetItems(const std::vector<ShopItemData>& items) {
    m_petItems = items;
    SyncThumbnailAtlases();
}

void ShopView::SyncThumbnailAtlases() {
    auto sync = [](const std::vector<ShopItemData>& items,
                   std::vector<std::shared_ptr<Animations::TextureAtlas>>& atlases) {
        if (atlases.size() != items.size()) atlases.assign(items.size(), nullptr);
        for (size_t i = 0; i < items.size(); ++i) {
            if (!atlases[i]) atlases[i] = AssetManager::GetInstance().GetAtlas(items[i].idleAtlasPath);
        }
    };
    sync(m_charItems, m_charThumbnails);
    sync(m_petItems, m_petThumbnails);
}

void ShopView::Show(int currentCoins) {
    m_currentCoins = currentCoins;
    m_visible = true;
    m_wantsBack = false;
    m_wantsBuy = false;
    m_activeTab = ShopTab::Characters;
    m_selectedIdx = 0;
    m_slideT = 0.0f;
    m_scrollY = m_scrollYDisp = 0.0f;
    m_shakeTimer = m_shakeOffset = 0.0f;
    m_notice.clear();
    m_noticeTimer = 0.0f;
    m_inputCooldown = 0.18f;
    for (int i = 0; i < (int)m_charItems.size(); ++i)
        if (m_charItems[i].isEquipped) { m_selectedIdx = i; break; }
    if (!m_charItems.empty()) LoadPreview(m_charItems[m_selectedIdx].idleAtlasPath);
}

void ShopView::MarkSelectedUnlocked() {
    auto& items = m_activeTab == ShopTab::Characters ? m_charItems : m_petItems;
    if (m_selectedIdx >= 0 && m_selectedIdx < (int)items.size()) items[m_selectedIdx].isUnlocked = true;
}

void ShopView::TriggerBuyShake() { m_shakeTimer = 0.35f; }

void ShopView::ShowNotice(const std::string& message, bool success) {
    m_notice = message;
    m_noticeSuccess = success;
    m_noticeTimer = 2.2f;
}

void ShopView::UpdateBtnAnim(BtnAnim& button, Rectangle rect, float dt) {
    button.hovered = CheckCollisionPointRec(GetMousePosition(), rect);
    const float t = std::min(1.0f, dt * 11.0f);
    button.scale = LerpValue(button.scale, button.hovered ? 1.045f : 1.0f, t);
    button.glowAlpha = LerpValue(button.glowAlpha, button.hovered ? 0.58f : 0.0f, t);
}

void ShopView::DrawGlowBorder(Rectangle rect, float alpha, Color color) {
    if (alpha <= 0.01f) return;
    for (int i = 1; i <= 3; ++i) {
        const float e = i * 3.0f;
        DrawRectangleRoundedLinesEx({rect.x-e,rect.y-e,rect.width+e*2,rect.height+e*2},
                                    0.15f, 8, 1.0f,
                                    Color{color.r,color.g,color.b,(unsigned char)(alpha*80.0f/i)});
    }
}

void ShopView::LoadPreview(const std::string& atlasPath) {
    if (atlasPath == m_loadedPreviewPath) return;
    m_loadedPreviewPath = atlasPath;
    m_previewAtlas = AssetManager::GetInstance().GetAtlas(atlasPath);
    m_previewAnim = Animations::Animator{};
    if (m_previewAtlas && m_previewAtlas->IsTextureLoaded()) {
        m_previewAnim.LoadClipsFromAtlas(*m_previewAtlas);
        for (const char* name : {"idle", "Idle", "IDLE", "default"}) {
            if (m_previewAnim.HasClip(name)) { m_previewAnim.Play(name, 1.0f, true); break; }
        }
    }
}

void ShopView::Update(float dt) {
    if (!m_visible) return;
    m_animTime += dt;
    m_slideT = std::min(1.0f, m_slideT + dt / 0.28f);
    m_inputCooldown = std::max(0.0f, m_inputCooldown - dt);
    m_noticeTimer = std::max(0.0f, m_noticeTimer - dt);
    if (m_previewAnim.IsPlaying()) m_previewAnim.Update(dt);

    if (m_shakeTimer > 0.0f) {
        m_shakeTimer = std::max(0.0f, m_shakeTimer - dt);
        m_shakeOffset = std::sin(m_shakeTimer * 68.0f) * 7.0f;
    } else m_shakeOffset = 0.0f;

    const ShopLayout l = BuildShopLayout(m_slideT);
    UpdateBtnAnim(m_backBtnAnim, l.back, dt);
    UpdateBtnAnim(m_buyBtnAnim, l.action, dt);
    UpdateBtnAnim(m_tabBtnAnim[0], l.tabs[0], dt);
    UpdateBtnAnim(m_tabBtnAnim[1], l.tabs[1], dt);

    auto& items = m_activeTab == ShopTab::Characters ? m_charItems : m_petItems;
    auto selectIndex = [&](int index) {
        if (index < 0 || index >= (int)items.size() || index == m_selectedIdx) return;
        m_selectedIdx = index;
        LoadPreview(items[index].idleAtlasPath);
        SoundManager::GetInstance().PlaySound("ui_hover");
    };
    auto switchTab = [&](ShopTab tab) {
        if (tab == m_activeTab) return;
        m_activeTab = tab;
        auto& newItems = m_activeTab == ShopTab::Characters ? m_charItems : m_petItems;
        m_selectedIdx = 0;
        for (int i = 0; i < (int)newItems.size(); ++i)
            if (newItems[i].isEquipped) { m_selectedIdx = i; break; }
        m_scrollY = m_scrollYDisp = 0.0f;
        if (!newItems.empty()) LoadPreview(newItems[m_selectedIdx].idleAtlasPath);
        SoundManager::GetInstance().PlaySound("ui_confirm");
    };

    const Vector2 mouse = GetMousePosition();
    const bool clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
    if (clicked) {
        if (m_backBtnAnim.hovered) m_wantsBack = true;
        else if (m_tabBtnAnim[0].hovered) switchTab(ShopTab::Characters);
        else if (m_tabBtnAnim[1].hovered) switchTab(ShopTab::Pets);
        else if (m_buyBtnAnim.hovered) m_wantsBuy = true;
        else if (CheckCollisionPointRec(mouse, l.grid)) {
            for (int i = 0; i < (int)items.size(); ++i) {
                Rectangle card = GridCard(l.grid, i);
                card.y -= m_scrollYDisp;
                if (CheckCollisionPointRec(mouse, card)) { selectIndex(i); break; }
            }
        }
    }

    if (CheckCollisionPointRec(mouse, l.grid)) {
        const float wheel = GetMouseWheelMove();
        const float totalHeight = items.size() <= 2 ? l.grid.height : GridCard(l.grid, (int)items.size()-1).y + GridCard(l.grid, (int)items.size()-1).height - l.grid.y;
        m_scrollY = std::clamp(m_scrollY - wheel * 80.0f, 0.0f, std::max(0.0f, totalHeight - l.grid.height));
    }
    m_scrollYDisp = LerpValue(m_scrollYDisp, m_scrollY, std::min(1.0f, dt * 14.0f));

    if (IsKeyPressed(KEY_ESCAPE)) m_wantsBack = true;
    if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_Q) || IsKeyPressed(KEY_E))
        switchTab(m_activeTab == ShopTab::Characters ? ShopTab::Pets : ShopTab::Characters);

    if (m_inputCooldown <= 0.0f && !items.empty()) {
        int delta = 0;
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) delta = -1;
        else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) delta = 1;
        else if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) delta = -2;
        else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) delta = 2;
        if (delta != 0) {
            const int oldRow = m_selectedIdx / 2;
            int next = std::clamp(m_selectedIdx + delta, 0, (int)items.size()-1);
            if ((delta == -1 || delta == 1) && next / 2 != oldRow) next = m_selectedIdx;
            selectIndex(next);
            m_inputCooldown = 0.12f;
        }
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            m_wantsBuy = true;
            m_inputCooldown = 0.16f;
        }
    }
}

void ShopView::Render() {
    if (!m_visible) return;
    RenderBackground();
    RenderTabBar();
    RenderGrid();
    RenderDetailPanel();
    RenderBuyButton();
    RenderBackButton();
    RenderCoinBar();
}

void ShopView::RenderBackground() {
    const ShopLayout l = BuildShopLayout(m_slideT);
    const Font font = m_fontLoaded ? m_font : GetFontDefault();
    DrawRectangleGradientV(0,0,GetScreenWidth(),GetScreenHeight(),Color{13,8,29,255},Color{4,3,12,255});
    for (int i = 0; i < 18; ++i) {
        const float x = std::fmod(i * 149.0f + m_animTime * (5.0f + i%4), (float)GetScreenWidth());
        const float y = std::fmod(i * 83.0f, (float)GetScreenHeight());
        DrawCircleV({x,y}, 1.0f + (i%3)*0.55f, Color{194,153,246,(unsigned char)(35+i%4*15)});
    }
    DrawRectangleRounded({l.panel.x+8,l.panel.y+10,l.panel.width,l.panel.height},0.035f,12,Color{0,0,0,135});
    DrawRectangleRounded(l.panel,0.035f,12,Color{22,16,39,248});
    DrawRectangleRoundedLinesEx(l.panel,0.035f,12,2.5f,Color{178,128,60,235});
    DrawLineEx({l.detail.x-8,l.detail.y},{l.detail.x-8,l.panel.y+l.panel.height-12},2.0f,Color{104,76,139,145});

    const char* title = "ARCANE EMPORIUM";
    const float size = std::clamp(l.panel.height*0.054f,20.0f,36.0f);
    const Vector2 measured = MeasureTextEx(font,title,size,1.4f);
    DrawTextEx(font,title,{l.panel.x+(l.panel.width-measured.x)*0.5f,l.panel.y+14},size,1.4f,Color{255,220,103,255});
    if (m_noticeTimer > 0.0f && !m_notice.empty()) {
        const float noticeSize = std::max(10.0f,size*0.38f);
        const Vector2 nm = MeasureTextEx(font,m_notice.c_str(),noticeSize,1.0f);
        Rectangle badge = {l.detail.x+(l.detail.width-nm.x-30)*0.5f,
                           l.action.y-noticeSize-26,nm.x+30,noticeSize+16};
        const Color accent = m_noticeSuccess ? Color{107,230,159,255} : Color{235,105,118,255};
        DrawRectangleRounded(badge,0.35f,8,Fade(accent,0.16f));
        DrawRectangleRoundedLinesEx(badge,0.35f,8,1.5f,Fade(accent,0.75f));
        DrawTextEx(font,m_notice.c_str(),{badge.x+15,badge.y+8},noticeSize,1.0f,accent);
    }
}

void ShopView::RenderCoinBar() {
    const ShopLayout l = BuildShopLayout(m_slideT);
    const Font font = m_fontLoaded ? m_font : GetFontDefault();
    DrawRectangleRounded(l.coin,0.3f,9,Color{37,28,57,250});
    DrawRectangleRoundedLinesEx(l.coin,0.3f,9,1.5f,Color{208,165,53,225});
    const float radius = l.coin.height*0.25f;
    const Vector2 center = {l.coin.x+radius+12,l.coin.y+l.coin.height*0.5f};
    DrawCircleV(center,radius,Color{255,205,40,255});
    DrawCircleLines((int)center.x,(int)center.y,radius,Color{255,235,130,255});
    const std::string value = std::to_string(m_currentCoins);
    const float size = std::max(11.0f,l.coin.height*0.42f);
    const Vector2 measured = MeasureTextEx(font,value.c_str(),size,1.0f);
    DrawTextEx(font,value.c_str(),{l.coin.x+l.coin.width-measured.x-12,l.coin.y+(l.coin.height-measured.y)*0.5f},size,1.0f,Color{255,232,122,255});
}

void ShopView::RenderTabBar() {
    const ShopLayout l = BuildShopLayout(m_slideT);
    const Font font = m_fontLoaded ? m_font : GetFontDefault();
    const char* labels[] = {"HEROES", "COMPANIONS"};
    for (int i = 0; i < 2; ++i) {
        const bool active = (int)m_activeTab == i;
        Rectangle rect = l.tabs[i];
        const float scale = m_tabBtnAnim[i].scale;
        rect.x -= rect.width*(scale-1)*0.5f; rect.y -= rect.height*(scale-1)*0.5f;
        rect.width *= scale; rect.height *= scale;
        DrawRectangleRounded(rect,0.22f,9,active?Color{82,57,119,255}:Color{37,29,57,245});
        DrawRectangleRoundedLinesEx(rect,0.22f,9,active?2.3f:1.2f,active?Color{255,211,85,255}:Color{105,80,137,210});
        const float size = std::clamp(rect.height*0.34f,10.0f,16.0f);
        const Vector2 measured = MeasureTextEx(font,labels[i],size,1.0f);
        DrawTextEx(font,labels[i],{rect.x+(rect.width-measured.x)*0.5f,rect.y+(rect.height-measured.y)*0.5f},size,1.0f,active?Color{255,232,142,255}:Color{192,180,212,235});
    }
}

void ShopView::RenderGrid() {
    const ShopLayout l = BuildShopLayout(m_slideT);
    const Font font = m_fontLoaded ? m_font : GetFontDefault();
    const auto& items = m_activeTab == ShopTab::Characters ? m_charItems : m_petItems;
    const auto& atlases = m_activeTab == ShopTab::Characters ? m_charThumbnails : m_petThumbnails;
    BeginScissorMode((int)l.grid.x,(int)l.grid.y,(int)l.grid.width,(int)l.grid.height);
    for (int i = 0; i < (int)items.size(); ++i) {
        Rectangle card = GridCard(l.grid,i); card.y -= m_scrollYDisp;
        const bool selected = i == m_selectedIdx;
        const bool owned = items[i].isUnlocked;
        if (selected) DrawRectangleRounded({card.x-5,card.y-5,card.width+10,card.height+10},0.10f,9,Color{255,196,66,38});
        DrawRectangleRounded(card,0.08f,9,selected?Color{61,45,88,255}:Color{31,25,48,250});
        DrawRectangleRoundedLinesEx(card,0.08f,9,selected?2.5f:1.2f,selected?Color{255,213,91,255}:Color{93,72,120,205});
        Rectangle portrait = {card.x+8,card.y+8,card.width-16,card.height*0.62f};
        DrawRectangleRounded(portrait,0.08f,7,Color{13,11,25,245});
        DrawCircleGradient({portrait.x+portrait.width*0.5f,portrait.y+portrait.height*0.60f},portrait.width*0.34f,
                           owned?Color{112,78,165,72}:Color{59,51,72,55},Color{10,8,20,0});
        if (i < (int)atlases.size()) DrawAtlasThumbnail(atlases[i],{portrait.x+8,portrait.y+4,portrait.width-16,portrait.height-8},owned?WHITE:Color{105,100,115,255});
        const float nameSize = std::clamp(card.height*0.085f,9.0f,15.0f);
        const Vector2 nameMeasure = MeasureTextEx(font,items[i].displayName.c_str(),nameSize,1.0f);
        DrawTextEx(font,items[i].displayName.c_str(),{card.x+(card.width-nameMeasure.x)*0.5f,card.y+card.height*0.68f},nameSize,1.0f,owned?RAYWHITE:Color{158,146,174,235});
        std::string tag;
        Color tagColor;
        if (items[i].isEquipped) { tag="EQUIPPED"; tagColor=Color{112,235,167,255}; }
        else if (owned) { tag="OWNED"; tagColor=Color{130,194,242,255}; }
        else { tag=std::to_string(items[i].price)+" COINS"; tagColor=Color{255,207,72,255}; }
        const float tagSize = std::max(8.0f,nameSize*0.76f);
        const Vector2 tagMeasure = MeasureTextEx(font,tag.c_str(),tagSize,1.0f);
        DrawTextEx(font,tag.c_str(),{card.x+(card.width-tagMeasure.x)*0.5f,card.y+card.height*0.84f},tagSize,1.0f,tagColor);
        if (!owned) DrawLockOverlay(portrait.x,portrait.y,portrait.width,portrait.height);
    }
    EndScissorMode();
}

void ShopView::DrawLockOverlay(float x,float y,float w,float h) {
    DrawRectangleRounded({x,y,w,h},0.08f,7,Color{3,3,8,145});
    const float size = std::min(w,h)*0.20f;
    const Vector2 c = {x+w*0.5f,y+h*0.48f};
    DrawRing(c,size*0.48f,size*0.68f,180,360,18,Color{184,154,211,245});
    DrawRectangleRounded({c.x-size*0.62f,c.y,size*1.24f,size*0.92f},0.18f,6,Color{73,54,98,250});
    DrawCircleV({c.x,c.y+size*0.38f},size*0.10f,Color{20,14,31,255});
}

void ShopView::RenderDetailPanel() {
    const ShopLayout l = BuildShopLayout(m_slideT);
    const Font font = m_fontLoaded ? m_font : GetFontDefault();
    const auto& items = m_activeTab == ShopTab::Characters ? m_charItems : m_petItems;
    if (items.empty()) return;
    const ShopItemData& item = items[std::clamp(m_selectedIdx,0,(int)items.size()-1)];
    const float pad = std::clamp(l.detail.width*0.055f,10.0f,24.0f);
    const float previewSize = std::min(l.detail.width*0.56f,l.detail.height*0.38f);
    Rectangle preview = {l.detail.x+(l.detail.width-previewSize)*0.5f,l.detail.y+10,previewSize,previewSize};
    DrawCircleGradient({preview.x+preview.width*0.5f,preview.y+preview.height*0.58f},preview.width*0.48f,Color{117,75,173,85},Color{13,10,25,0});
    DrawEllipse((int)(preview.x+preview.width*0.5f),(int)(preview.y+preview.height*0.83f),preview.width*0.30f,preview.height*0.055f,Fade(BLACK,0.55f));
    if (m_previewAnim.HasTexture()) {
        Rectangle src=m_previewAnim.GetCurrentSrcRect(); Texture2D* texture=m_previewAnim.GetCurrentTexture();
        if (texture && src.width!=0 && src.height!=0) {
            const float scale=std::min(preview.width/std::fabs(src.width),preview.height/std::fabs(src.height))*0.74f;
            const float dw=std::fabs(src.width)*scale,dh=std::fabs(src.height)*scale;
            DrawTexturePro(*texture,src,{preview.x+(preview.width-dw)*0.5f,preview.y+(preview.height-dh)*0.5f,dw,dh},{0,0},0,WHITE);
        }
    }
    const float nameSize=std::clamp(l.detail.height*0.052f,15.0f,27.0f);
    const Vector2 nameMeasure=MeasureTextEx(font,item.displayName.c_str(),nameSize,1.0f);
    float y=preview.y+preview.height+5;
    DrawTextEx(font,item.displayName.c_str(),{l.detail.x+(l.detail.width-nameMeasure.x)*0.5f,y},nameSize,1.0f,Color{255,223,119,255});
    y += nameSize*1.35f;

    auto stat=[&](const char* label,int value,Color color) {
        const float fs=std::clamp(l.detail.height*0.025f,9.0f,13.0f);
        DrawTextEx(font,label,{l.detail.x+pad,y},fs,1.0f,Color{195,184,215,240});
        const float barX=l.detail.x+l.detail.width*0.29f;
        const float barW=l.detail.width-pad-(barX-l.detail.x)-30;
        Rectangle track={barX,y+fs*0.20f,barW,fs*0.55f};
        DrawRectangleRounded(track,0.5f,7,Color{13,10,25,255});
        DrawRectangleRounded({track.x,track.y,track.width*std::min(value/200.0f,1.0f),track.height},0.5f,7,color);
        const std::string val=std::to_string(value);
        DrawTextEx(font,val.c_str(),{track.x+track.width+6,y},fs,1.0f,RAYWHITE);
        y += fs*1.65f;
    };
    stat("HP",item.statHP,Color{77,199,111,255});
    stat(m_activeTab==ShopTab::Characters?"ATK":"POWER",item.statATK,Color{221,91,99,255});
    stat("SPEED",item.statSpeed,Color{77,174,226,255});
    y += 4;
    DrawWrapped(font,item.description,{l.detail.x+pad,y,l.detail.width-pad*2,l.action.y-y-9},
                std::clamp(l.detail.height*0.022f,9.0f,13.0f),Color{170,158,194,230});
}

void ShopView::RenderBuyButton() {
    const ShopLayout l = BuildShopLayout(m_slideT);
    const Font font = m_fontLoaded ? m_font : GetFontDefault();
    const auto& items = m_activeTab == ShopTab::Characters ? m_charItems : m_petItems;
    if (items.empty()) return;
    const ShopItemData& item=items[std::clamp(m_selectedIdx,0,(int)items.size()-1)];
    Rectangle rect=l.action; rect.x+=m_shakeOffset;
    const float scale=m_buyBtnAnim.scale;
    rect.x-=rect.width*(scale-1)*0.5f; rect.y-=rect.height*(scale-1)*0.5f; rect.width*=scale; rect.height*=scale;
    const bool canAfford=m_currentCoins>=item.price;
    Color fill=item.isEquipped?Color{40,86,69,255}:item.isUnlocked?Color{55,70,111,255}:canAfford?Color{94,61,132,255}:Color{58,42,65,255};
    Color border=item.isEquipped?Color{104,230,164,255}:item.isUnlocked?Color{117,188,239,255}:canAfford?Color{255,211,85,255}:Color{173,91,104,220};
    DrawRectangleRounded(rect,0.22f,9,fill);
    DrawRectangleRoundedLinesEx(rect,0.22f,9,2.2f,border);
    DrawGlowBorder(rect,m_buyBtnAnim.glowAlpha,border);
    std::string label=item.isEquipped?"EQUIPPED":item.isUnlocked?"EQUIP":"BUY  "+std::to_string(item.price)+" COINS";
    const float size=std::clamp(rect.height*0.33f,11.0f,17.0f);
    const Vector2 measured=MeasureTextEx(font,label.c_str(),size,1.0f);
    DrawTextEx(font,label.c_str(),{rect.x+(rect.width-measured.x)*0.5f,rect.y+(rect.height-measured.y)*0.5f},size,1.0f,item.isEquipped?Color{155,245,193,255}:RAYWHITE);
}

void ShopView::RenderBackButton() {
    const ShopLayout l=BuildShopLayout(m_slideT);
    const Font font=m_fontLoaded?m_font:GetFontDefault();
    Rectangle rect=l.back;
    DrawRectangleRounded(rect,0.25f,8,m_backBtnAnim.hovered?Color{73,51,104,255}:Color{36,28,55,245});
    DrawRectangleRoundedLinesEx(rect,0.25f,8,m_backBtnAnim.hovered?2.0f:1.1f,m_backBtnAnim.hovered?Color{255,212,89,255}:Color{109,83,139,215});
    const char* label="< BACK"; const float size=std::max(10.0f,rect.height*0.34f);
    const Vector2 measured=MeasureTextEx(font,label,size,1.0f);
    DrawTextEx(font,label,{rect.x+(rect.width-measured.x)*0.5f,rect.y+(rect.height-measured.y)*0.5f},size,1.0f,RAYWHITE);
}
