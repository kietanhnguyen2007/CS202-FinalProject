#include "Controller/MenuController.h"
#include "Controller/GameController.h"
#include "Controller/MapBuilderController.h"
#include "Controller/ShopController.h"
#include "View/Renderer.h"
#include "View/MenuView.h"
#include "View/GameView.h"
#include "View/HUDView.h"
#include "View/AssetManager.h"
#include "View/OptionsView.h"
#include "Utils/Constants.h"
#include "Systems/WindowManager.h"
#include "raylib.h"
#include <filesystem>
#include <vector>
#include <string>
#include <cmath>
#include <cstdio>

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Apple Knight Adventure");
    SetExitKey(0); // Disable ESC to quit so we can use it for Pause Menu
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetWindowMinSize(640, 360);  // minimum 16:9 at half base resolution
    WindowManager::GetInstance().Init(SCREEN_WIDTH, SCREEN_HEIGHT);
    SetTargetFPS(60);

    if (!View::Renderer::GetInstance().Init()) {
        CloseWindow();
        return 1;
    }

    // --- Multithreaded Asset Preloading ---
    std::vector<std::string> jsonFiles;
    for (const auto& entry : std::filesystem::recursive_directory_iterator("assets/textures")) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            jsonFiles.push_back(entry.path().string());
        }
    }

    auto& assetManager = View::AssetManager::GetInstance();
    assetManager.StartLoading(jsonFiles);

    // Load game_font for loading screen
    Font loadingFont = LoadFont("assets/fonts/game_font.ttf");
    bool hasFontLoaded = (loadingFont.texture.id != 0);

    // Load background texture (forest/back.png) for loading screen
    Texture2D loadBg = {0};
    if (FileExists("assets/textures/backgrounds/forest/back.png"))
        loadBg = LoadTexture("assets/textures/backgrounds/forest/back.png");

    // Load darkDwellers bar asset for progress bar
    Texture2D barBg   = {0}; // empty bar frame
    Texture2D barFill = {0}; // filled bar
    if (FileExists("assets/ui/darkDwellers/20251118darkDwellersBarA.png"))
        barBg   = LoadTexture("assets/ui/darkDwellers/20251118darkDwellersBarA.png");
    if (FileExists("assets/ui/darkDwellers/20251118darkDwellersBarB.png"))
        barFill = LoadTexture("assets/ui/darkDwellers/20251118darkDwellersBarB.png");

    float loadingTime = 0.0f;

    while (!WindowShouldClose() && !assetManager.IsLoadingComplete()) {
        WindowManager::GetInstance().Update(); // Catch resize events during loading!
        assetManager.UpdateMainThread();
        float dt = GetFrameTime();
        loadingTime += dt;

        // Always use actual screen size (window may be resizable)
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();

        float displayProg = assetManager.GetDisplayProgress();
        float realProg    = assetManager.GetProgress();
        int   percent     = static_cast<int>(realProg * 100.0f);

        std::string assetName = assetManager.GetCurrentAssetName();
        if (assetName.empty()) assetName = "Initializing...";
        else assetName = "Loading: " + assetName;

        BeginDrawing();
        ClearBackground(Color{10, 8, 20, 255});

        // ── Background (parallax forest stretched to fill) ─────────────────
        if (loadBg.id != 0) {
            float bgScale = std::max((float)sw / loadBg.width, (float)sh / loadBg.height);
            int dstW = (int)(loadBg.width  * bgScale);
            int dstH = (int)(loadBg.height * bgScale);
            int bgX  = (sw - dstW) / 2;
            int bgY  = (sh - dstH) / 2;
            DrawTexturePro(loadBg,
                {0,0,(float)loadBg.width,(float)loadBg.height},
                {(float)bgX,(float)bgY,(float)dstW,(float)dstH},
                {0,0}, 0.f, Color{255,255,255,80});
        }

        // ── Dark overlay vignette ─────────────────────────────────────────
        DrawRectangle(0, 0, sw, sh, Color{5, 4, 12, 180});
        for (int rad = sh; rad > 0; rad -= 30) {
            unsigned char a = (unsigned char)(60.f * (1.f - (float)rad / sh));
            DrawCircle(sw/2, sh/2, (float)rad, Color{0,0,0,a});
        }

        // ── Title (game font, screen-relative size) ───────────────────────
        const char* title = "Apple Knight Adventure";
        int titleSize = (int)(sh * 0.075f); if (titleSize < 20) titleSize = 20;
        if (hasFontLoaded) {
            Vector2 measured = MeasureTextEx(loadingFont, title, (float)titleSize, 2.f);
            float tx = (sw - measured.x) * 0.5f;
            float ty = sh * 0.28f;
            DrawTextEx(loadingFont, title, {tx+3,ty+3}, (float)titleSize, 2.f, Color{0,0,0,130});
            DrawTextEx(loadingFont, title, {tx,ty},     (float)titleSize, 2.f, Color{255,230,80,255});
        } else {
            int tw = MeasureText(title, titleSize);
            DrawText(title, sw/2 - tw/2 + 2, (int)(sh*0.28f) + 2, titleSize, Color{0,0,0,130});
            DrawText(title, sw/2 - tw/2,     (int)(sh*0.28f),     titleSize, Color{255,230,80,255});
        }

        // ── Asset name label ──────────────────────────────────────────────
        int nameFs = (int)(sh * 0.025f); if (nameFs < 10) nameFs = 10;
        float barY = sh * 0.58f;
        if (hasFontLoaded) {
            Vector2 nm = MeasureTextEx(loadingFont, assetName.c_str(), (float)nameFs, 1.f);
            DrawTextEx(loadingFont, assetName.c_str(),
                       {(sw - nm.x)*0.5f, barY - nameFs - sh*0.015f},
                       (float)nameFs, 1.f, Color{180,165,210,200});
        } else {
            int nw = MeasureText(assetName.c_str(), nameFs);
            DrawText(assetName.c_str(), sw/2-nw/2, (int)(barY - nameFs - sh*0.015f),
                     nameFs, Color{180,165,210,200});
        }

        // ── Progress bar using darkDwellers bar asset ─────────────────────
        float barW = sw * 0.55f; // responsive width
        float barH = (float)(sh) * 0.04f; if (barH < 16) barH = 16;
        float barX = (sw - barW) * 0.5f;

        if (barBg.id != 0) {
            // Draw bar background texture (stretched)
            DrawTexturePro(barBg,
                {0,0,(float)barBg.width,(float)barBg.height},
                {barX, barY, barW, barH}, {0,0}, 0.f, WHITE);
        } else {
            // Fallback: plain rect
            DrawRectangle((int)(barX-2),(int)(barY-2),(int)(barW+4),(int)(barH+4),
                          Color{60,50,80,200});
            DrawRectangle((int)barX,(int)barY,(int)barW,(int)barH, Color{20,15,35,255});
        }

        // Fill portion
        int fillW = (int)(barW * displayProg);
        if (fillW > 2) {
            if (barFill.id != 0) {
                // Clip fill texture to filled portion
                float fillSrcW = barFill.width * displayProg;
                DrawTexturePro(barFill,
                    {0,0,fillSrcW,(float)barFill.height},
                    {barX, barY, (float)fillW, barH}, {0,0}, 0.f, WHITE);
            } else {
                DrawRectangleGradientH((int)barX,(int)barY, fillW,(int)barH,
                    Color{100,60,180,255}, Color{255,200,50,255});
            }
            // Shimmer on fill edge
            float shimmer = (sinf(loadingTime * 7.f) + 1.f) * 0.5f;
            unsigned char sa = (unsigned char)(80 + shimmer * 140);
            DrawRectangle((int)(barX+fillW-5),(int)barY, 5,(int)barH,
                          Color{255,255,255,sa});
        }

        // ── Percent text (game font) ──────────────────────────────────────
        int pctFs = (int)(sh * 0.032f); if (pctFs < 12) pctFs = 12;
        char pctBuf[16]; snprintf(pctBuf, sizeof(pctBuf), "%d%%", percent);
        float pctY = barY + barH + sh * 0.015f;
        if (hasFontLoaded) {
            Vector2 pm = MeasureTextEx(loadingFont, pctBuf, (float)pctFs, 1.f);
            DrawTextEx(loadingFont, pctBuf, {(sw-pm.x)*0.5f, pctY},
                       (float)pctFs, 1.f, Color{220,205,255,230});
        } else {
            int pw = MeasureText(pctBuf, pctFs);
            DrawText(pctBuf, sw/2-pw/2, (int)pctY, pctFs, Color{220,205,255,230});
        }

        EndDrawing();
    }
    if (hasFontLoaded)  UnloadFont(loadingFont);
    if (loadBg.id != 0) UnloadTexture(loadBg);
    if (barBg.id != 0)  UnloadTexture(barBg);
    if (barFill.id != 0) UnloadTexture(barFill);
    // ---------------------------------------

    auto& menu  = MenuController::GetInstance();
    auto& game  = GameController::GetInstance();
    auto& shop  = ShopController::GetInstance();
    auto& opts  = View::OptionsView::GetInstance();
    menu.Init();
    game.Init();
    shop.Init();
    opts.Init();

    bool inGame       = false;
    bool inMapBuilder = false;
    bool inShop       = false;
    bool inOptions    = false;

    while (!WindowShouldClose()) {
        WindowManager::GetInstance().Update();
        float dt = GetFrameTime();

        if (!inGame && !inMapBuilder && !inShop && !inOptions) {
            menu.Update(dt);

            if (menu.ShouldOpenShop()) {
                menu.ResetFlags();
                inShop = true;
                shop.Open();
            } else if (menu.ShouldOpenOptions()) {
                menu.ResetFlags();
                inOptions = true;
                opts.SetVisible(true);
            } else if (menu.ShouldStartGame()) {
                inGame = true;
                game.StartLevel(1);
            } else if (menu.ShouldOpenMapBuilder()) {
                inMapBuilder = true;
                MapBuilderController::GetInstance().StartEditor();
            } else if (menu.ShouldQuit()) {
                break;
            }

            BeginDrawing();
            ClearBackground(BLACK);
            View::Renderer::GetInstance().BeginFrame();
            View::MenuView::GetInstance().Update(dt, menu.GetSelected());
            View::MenuView::GetInstance().Render();
            View::Renderer::GetInstance().EndFrameAndFlush();
            EndDrawing();
        } else if (inShop) {
            BeginDrawing();
            ClearBackground(BLACK);
            shop.Update(dt);
            if (shop.ShouldReturnToMenu()) {
                inShop = false;
                menu.ShowMainMenu();
            }
            EndDrawing();
        } else if (inOptions) {
            opts.Update(dt);
            BeginDrawing();
            ClearBackground(BLACK);
            // Draw menu in background, then options overlay
            View::Renderer::GetInstance().BeginFrame();
            View::MenuView::GetInstance().Update(dt, menu.GetSelected());
            View::MenuView::GetInstance().Render();
            View::Renderer::GetInstance().EndFrameAndFlush();
            opts.Render();
            EndDrawing();
            if (opts.WantsBack()) {
                opts.ClearWantsBack();
                opts.SetVisible(false);
                inOptions = false;
                menu.ShowMainMenu();
            }
        } else if (inMapBuilder) {
            MapBuilderController::GetInstance().Update(dt);
            if (MapBuilderController::GetInstance().ShouldReturnToMenu()) {
                inMapBuilder = false;
                menu.ShowMainMenu();
                
                // Pump events
                BeginDrawing();
                ClearBackground(BLACK);
                View::Renderer::GetInstance().BeginFrame();
                View::MenuView::GetInstance().Update(dt, menu.GetSelected());
                View::MenuView::GetInstance().Render();
                View::Renderer::GetInstance().EndFrameAndFlush();
                EndDrawing();
            } else if (MapBuilderController::GetInstance().WantsToPlaytest()) {
                inMapBuilder = false;
                inGame = true;
            }
        } else {
            game.Update(dt);
            if (game.ShouldReturnToMenu()) {
                bool wasPlaytest = game.IsPlaytest();
                game.Shutdown();
                inGame = false;
                
                if (wasPlaytest) {
                    inMapBuilder = true;
                    MapBuilderController::GetInstance().ResumeEditor();
                } else {
                    menu.ShowMainMenu();
                    
                    // Render one frame of menu to consume the input and pump events
                    BeginDrawing();
                    ClearBackground(BLACK);
                    View::Renderer::GetInstance().BeginFrame();
                    View::MenuView::GetInstance().Update(dt, menu.GetSelected());
                    View::MenuView::GetInstance().Render();
                    View::Renderer::GetInstance().EndFrameAndFlush();
                    EndDrawing();
                }
                
                continue;
            }

            BeginDrawing();
            ClearBackground(BLACK);
            game.Render();
            EndDrawing();
        }
    }

    if (inGame) {
        game.Shutdown();
    }
    menu.Shutdown();
    View::GameView::GetInstance().Shutdown();
    View::HUDView::GetInstance().Shutdown();
    View::Renderer::GetInstance().Shutdown();
    CloseWindow();
    return 0;
}
