#include "Controller/MenuController.h"
#include "Controller/GameController.h"
#include "Controller/MapBuilderController.h"
#include "Controller/ShopController.h"
#include "Controller/PrepareController.h"
#include "View/Renderer.h"
#include "View/MenuView.h"
#include "View/GameView.h"
#include "View/HUDView.h"
#include "View/SkillBarView.h"
#include "View/AssetManager.h"
#include "View/OptionsView.h"
#include "View/PrepareView.h"
#include "View/MapBuilderView.h"
#include "View/InventoryView.h"
#include "View/InteractPrompt.h"
#include "View/MinimapView.h"
#include "View/UIStateManager.h"
#include "Utils/Constants.h"
#include "Systems/WindowManager.h"
#include "Systems/SoundManager.h"
#include "Systems/AchievementManager.h"
#include "raylib.h"
#include <filesystem>
#include <vector>
#include <string>
#include <cmath>
#include <cstdio>
#include <algorithm>

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
    std::sort(jsonFiles.begin(), jsonFiles.end());

    auto& assetManager = View::AssetManager::GetInstance();
    assetManager.StartLoading(jsonFiles);

    // Load game_font for loading screen
    Font loadingFont = LoadFont("assets/fonts/game_font.ttf");
    bool hasFontLoaded = (loadingFont.texture.id != 0);

    // Full-HD Rimuru artwork used as the loading backdrop.
    Texture2D loadBg = {0};
    if (FileExists("assets/ui/loading/rimuru_background.png"))
        loadBg = LoadTexture("assets/ui/loading/rimuru_background.png");

    // Load darkDwellers bar asset for progress bar
    Texture2D barBg   = {0}; // empty bar frame
    Texture2D barFill = {0}; // filled bar
    if (FileExists("assets/ui/darkDwellers/20251118darkDwellersBarA.png"))
        barBg   = LoadTexture("assets/ui/darkDwellers/20251118darkDwellersBarA.png");
    if (FileExists("assets/ui/darkDwellers/20251118darkDwellersBarB.png"))
        barFill = LoadTexture("assets/ui/darkDwellers/20251118darkDwellersBarB.png");

    float loadingTime = 0.0f;
    float completionHold = 0.0f;

    while (!WindowShouldClose()) {
        WindowManager::GetInstance().Update(); // Catch resize events during loading!
        assetManager.UpdateMainThread();
        float dt = GetFrameTime();
        loadingTime += dt;

        // Always use actual screen size (window may be resizable)
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();

        const bool assetsComplete = assetManager.IsLoadingComplete();
        float displayProg = std::clamp(assetManager.GetDisplayProgress(), 0.0f, 1.0f);
        float realProg    = std::clamp(assetManager.GetProgress(), 0.0f, 1.0f);
        if (assetsComplete && displayProg >= 0.995f) completionHold += dt;
        else completionHold = 0.0f;
        int percent = std::clamp(static_cast<int>(displayProg * 100.0f + 0.5f), 0, 100);

        std::string assetName = assetManager.GetCurrentAssetName();
        if (assetName.empty()) assetName = "Preparing the adventure";
        const char* stage = realProg < 0.20f ? "FORGING THE WORLD"
                          : realProg < 0.52f ? "AWAKENING THE HEROES"
                          : realProg < 0.82f ? "SUMMONING CREATURES"
                          : assetsComplete ? "READYING YOUR ADVENTURE"
                                           : "POLISHING THE REALMS";

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
                {0,0}, 0.f, Color{255,255,255,115});
        }

        // ── Dark overlay vignette ─────────────────────────────────────────
        DrawRectangle(0, 0, sw, sh, Color{3, 8, 24, 76});
        const float contentW = std::min(sw * 0.72f, 900.0f);

        // ── Title (game font, screen-relative size) ───────────────────────
        const char* title = "APPLE KNIGHT ADVENTURE";
        float titleSize = std::clamp(sh * 0.058f, 20.0f, 42.0f);
        Font drawFont = hasFontLoaded ? loadingFont : GetFontDefault();
        while (titleSize > 15.0f && MeasureTextEx(drawFont,title,titleSize,1.6f).x > contentW) titleSize -= 1.0f;
        const float titleY = sh * 0.065f;
        if (hasFontLoaded) {
            Vector2 measured = MeasureTextEx(loadingFont, title, titleSize, 1.6f);
            float tx = (sw - measured.x) * 0.5f;
            DrawTextEx(loadingFont, title, {tx+3,titleY+3}, titleSize, 1.6f, Color{0,0,0,150});
            DrawTextEx(loadingFont, title, {tx,titleY}, titleSize, 1.6f, Color{255,226,113,255});
        } else {
            Vector2 measured = MeasureTextEx(drawFont,title,titleSize,1.0f);
            DrawTextEx(drawFont,title,{(sw-measured.x)*0.5f,titleY},titleSize,1.0f,Color{255,226,113,255});
        }

        // ── Asset name label ──────────────────────────────────────────────
        float nameFs = std::clamp(sh * 0.016f, 9.0f, 13.0f);
        while (nameFs > 8.0f && MeasureTextEx(drawFont,assetName.c_str(),nameFs,1.0f).x > contentW) nameFs -= 1.0f;
        const float stageFs = std::clamp(sh * 0.024f, 12.0f, 18.0f);
        Vector2 stageSize = MeasureTextEx(drawFont,stage,stageFs,1.0f);
        const float stageY = sh * 0.705f;
        DrawTextEx(drawFont,stage,{(sw-stageSize.x)*0.5f+2,stageY+2},stageFs,1.0f,Color{0,0,0,180});
        DrawTextEx(drawFont,stage,{(sw-stageSize.x)*0.5f,stageY},stageFs,1.0f,Color{235,243,255,255});
        Vector2 nm = MeasureTextEx(drawFont,assetName.c_str(),nameFs,1.0f);
        DrawTextEx(drawFont,assetName.c_str(),{(sw-nm.x)*0.5f+1,sh*0.753f+1},nameFs,1.0f,Color{0,0,0,170});
        DrawTextEx(drawFont,assetName.c_str(),{(sw-nm.x)*0.5f,sh*0.753f},nameFs,1.0f,Color{199,217,241,245});
        float barY = sh * 0.795f;

        // ── Progress bar using darkDwellers bar asset ─────────────────────
        float barW = std::min(sw * 0.60f, 760.0f);
        float barH = std::clamp(sh * 0.032f, 15.0f, 25.0f);
        float barX = (sw - barW) * 0.5f;

        if (barBg.id != 0) {
            // Draw bar background texture (stretched)
            DrawTexturePro(barBg,
                {0,0,(float)barBg.width,(float)barBg.height},
                {barX, barY, barW, barH}, {0,0}, 0.f, WHITE);
        } else {
            // Fallback: plain rect
            DrawRectangleRounded({barX-3,barY-3,barW+6,barH+6},0.5f,9,Color{111,81,146,220});
            DrawRectangleRounded({barX,barY,barW,barH},0.5f,9,Color{8,7,17,255});
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
                    Color{106,65,175,255}, Color{255,196,62,255});
            }
            // Shimmer on fill edge
            float shimmer = (sinf(loadingTime * 7.f) + 1.f) * 0.5f;
            unsigned char sa = (unsigned char)(80 + shimmer * 140);
            DrawRectangle((int)(barX+fillW-5),(int)barY, 5,(int)barH,
                          Color{255,255,255,sa});
        }

        // ── Percent text (game font) ──────────────────────────────────────
        int pctFs = (int)std::clamp(sh * 0.027f, 13.0f, 21.0f);
        char pctBuf[16]; snprintf(pctBuf, sizeof(pctBuf), "%d%%", percent);
        float pctY = barY + barH + 9.0f;
        if (hasFontLoaded) {
            Vector2 pm = MeasureTextEx(loadingFont, pctBuf, (float)pctFs, 1.f);
            DrawTextEx(loadingFont, pctBuf, {(sw-pm.x)*0.5f, pctY},
                       (float)pctFs, 1.f, Color{220,205,255,230});
        } else {
            int pw = MeasureText(pctBuf, pctFs);
            DrawText(pctBuf, sw/2-pw/2, (int)pctY, pctFs, Color{220,205,255,230});
        }

        EndDrawing();
        if (assetsComplete && displayProg >= 0.995f && completionHold >= 0.32f &&
            loadingTime >= 2.0f) break;
    }
    if (hasFontLoaded)  UnloadFont(loadingFont);
    if (loadBg.id != 0) UnloadTexture(loadBg);
    if (barBg.id != 0)  UnloadTexture(barBg);
    if (barFill.id != 0) UnloadTexture(barFill);
    // ---------------------------------------

    // The user may close the window during asset loading. Do not continue
    // initializing the whole game just to tear it down immediately.
    if (WindowShouldClose()) {
        SetTraceLogLevel(LOG_WARNING);
        assetManager.Shutdown();
        View::Renderer::GetInstance().Shutdown();
        CloseWindow();
        return 0;
    }

    auto& menu  = MenuController::GetInstance();
    auto& game  = GameController::GetInstance();
    auto& shop  = ShopController::GetInstance();
    auto& opts  = View::OptionsView::GetInstance();
    menu.Init();
    AchievementManager::GetInstance().Init();
    game.Init();
    shop.Init();
    opts.Init();

    bool inGame       = false;
    bool inMapBuilder = false;
    bool inShop       = false;
    bool inOptions    = false;
    bool inPrepare    = false;

    while (!WindowShouldClose()) {
        WindowManager::GetInstance().Update();
        float dt = GetFrameTime();
        SoundManager::GetInstance().Update(dt);
        AchievementManager::GetInstance().Update(dt);

        if (!inGame && !inMapBuilder && !inShop && !inOptions && !inPrepare) {
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
                menu.ResetFlags();
                inPrepare = true;
                PrepareController::GetInstance().Init();
                PrepareController::GetInstance().Open(menu.GetSelectedLevel());
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
            AchievementManager::GetInstance().RenderPopup();
            View::Renderer::GetInstance().EndFrameAndFlush();
            EndDrawing();
        } else if (inPrepare) {
            auto& prep = PrepareController::GetInstance();
            prep.Update(dt);
            
            if (prep.ShouldReturnToMenu()) {
                inPrepare = false;
                menu.ResetFlags();
                // Menu remains in LevelSelect mode
            } else if (prep.ShouldStartGame()) {
                inPrepare = false;
                inGame = true;
                game.ConfigureLocalCoop(prep.IsLocalCoop(), prep.GetSecondPlayerClass());
                game.StartLevel(prep.GetLevelId());
            }
            
            BeginDrawing();
            ClearBackground(BLACK);
            View::PrepareView::GetInstance().Render();
            AchievementManager::GetInstance().RenderPopup();
            EndDrawing();
        } else if (inShop) {
            BeginDrawing();
            ClearBackground(BLACK);
            shop.Update(dt);
            AchievementManager::GetInstance().RenderPopup();
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
            AchievementManager::GetInstance().RenderPopup();
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
            AchievementManager::GetInstance().RenderPopup();
            EndDrawing();
        }
    }

    // Raylib logs one INFO line for every released GPU object. With hundreds of
    // atlases this console I/O alone can noticeably delay exit; warnings and
    // errors remain enabled.
    SetTraceLogLevel(LOG_WARNING);

    // Release all gameplay/editor references first. GameView preloads character
    // atlases even when no level was opened, so this must run unconditionally.
    game.Shutdown();
    MapBuilderController::GetInstance().ExitEditor();

    // Drop view-owned atlases and textures while the OpenGL context is alive.
    View::UIStateManager::GetInstance().Shutdown();
    View::PrepareView::GetInstance().Shutdown();
    shop.Shutdown();
    opts.Shutdown();
    View::InventoryView::GetInstance().Shutdown();
    View::InteractPrompt::GetInstance().Shutdown();
    View::SkillBarView::GetInstance().Shutdown();
    View::HUDView::GetInstance().Shutdown();
    View::MapBuilderView::GetInstance().Shutdown();
    View::GameView::GetInstance().Shutdown();
    View::MinimapView::GetInstance().Shutdown();
    menu.Shutdown();
    AchievementManager::GetInstance().Shutdown();

    // Asset atlases and audio devices must be gone before the renderer/window.
    assetManager.Shutdown();
    SoundManager::GetInstance().CloseAudio();
    View::Renderer::GetInstance().Shutdown();
    CloseWindow();
    return 0;
}
