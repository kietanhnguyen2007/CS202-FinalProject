#include "Controller/MenuController.h"
#include "Controller/GameController.h"
#include "Controller/MapBuilderController.h"
#include "Controller/ShopController.h"
#include "Controller/PrepareController.h"
#include "Survival3D/Controller/SurvivalController.h"
#include "Survival3D/View/SurvivalView.h"
#include "Survival3D/Systems/SurvivalRunService.h"
#include "View/Renderer.h"
#include "View/MenuView.h"
#include "View/GameView.h"
#include "View/HUDView.h"
#include "View/SkillBarView.h"
#include "View/AssetManager.h"
#include "View/OptionsView.h"
#include "View/PrepareView.h"
#include "View/MapBuilderView.h"
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

#if defined(_WIN32)
// Hybrid-GPU laptops otherwise tend to start this OpenGL executable on the
// integrated adapter. Both vendors recognize these exports as a request for
// the high-performance GPU; systems without switchable graphics ignore them.
extern "C" {
__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

int main() {
    // Present in step with the display. SetTargetFPS alone only sleeps between
    // frames, so frames still land mid-refresh and tear; the 3D arena shows it
    // most because the camera pans continuously.
    SetConfigFlags(FLAG_VSYNC_HINT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Apple Knight Adventure");
    SetExitKey(0); // Disable ESC to quit so we can use it for Pause Menu
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetWindowMinSize(960, 540);  // minimum 3/4 base resolution to prevent UI layout issues
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
    auto& survivalView = Survival3D::SurvivalView::GetInstance();
    survivalView.BeginStartupAssetLoading();

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
    float combinedDisplayProgress = 0.0f;

    while (!WindowShouldClose()) {
        WindowManager::GetInstance().Update(); // Catch resize events during loading!
        assetManager.UpdateMainThread();
        float dt = GetFrameTime();
        loadingTime += dt;

        // Raylib model/texture uploads must stay on the render thread. Process
        // one Survival3D item per frame after the 2D atlas queue completes so
        // the loading screen remains responsive and reports each real asset.
        const bool atlasesComplete = assetManager.IsLoadingComplete();
        if (atlasesComplete && !survivalView.IsStartupAssetLoadingComplete())
            survivalView.LoadNextStartupAsset();

        // Always use actual screen size (window may be resizable)
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();

        const float atlasProgress = std::clamp(assetManager.GetProgress(), 0.0f, 1.0f);
        const float survivalProgress = std::clamp(
            survivalView.GetStartupAssetLoadingProgress(), 0.0f, 1.0f);
        // The final 28% is intentionally reserved for Survival3D. Counting
        // each GLB as one tiny item beside hundreds of sprite atlases would
        // hide the expensive 3D phase and make the bar misleading.
        const float realProg = 0.72f * atlasProgress
                             + (atlasesComplete ? 0.28f * survivalProgress : 0.0f);
        const float smoothing = std::clamp(dt * 8.0f, 0.0f, 1.0f);
        combinedDisplayProgress += (realProg - combinedDisplayProgress) * smoothing;
        float displayProg = std::clamp(combinedDisplayProgress, 0.0f, 1.0f);
        const bool assetsComplete = atlasesComplete
            && survivalView.IsStartupAssetLoadingComplete();
        if (assetsComplete && displayProg >= 0.995f) completionHold += dt;
        else completionHold = 0.0f;
        int percent = std::clamp(static_cast<int>(displayProg * 100.0f + 0.5f), 0, 100);

        std::string assetName = atlasesComplete
            ? survivalView.GetStartupAssetName()
            : assetManager.GetCurrentAssetName();
        if (assetName.empty()) assetName = "Preparing the adventure";
        const char* stage = nullptr;
        if (!atlasesComplete) {
            stage = realProg < 0.20f ? "FORGING THE WORLD"
                  : realProg < 0.52f ? "AWAKENING THE HEROES"
                                     : "SUMMONING CREATURES";
        } else if (survivalProgress < 0.40f) {
            stage = "RAISING THE 3D ARENA";
        } else if (survivalProgress < 0.74f) {
            stage = "AWAKENING 3D HEROES";
        } else if (!assetsComplete) {
            stage = "CHARGING 3D SKILLS";
        } else {
            stage = "READYING YOUR ADVENTURE";
        }

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
        survivalView.Shutdown();
        assetManager.Shutdown();
        View::Renderer::GetInstance().Shutdown();
        CloseWindow();
        return 0;
    }

    auto& menu  = MenuController::GetInstance();
    auto& game  = GameController::GetInstance();
    auto& shop  = ShopController::GetInstance();
    auto& opts  = View::OptionsView::GetInstance();
    auto& survival = Survival3D::SurvivalController::GetInstance();
    menu.Init();
    AchievementManager::GetInstance().Init();
    game.Init();
    shop.Init();
    opts.Init();
    survival.Init();
    Survival3D::SurvivalRunService::GetInstance().Init();

    bool inGame       = false;
    bool inMapBuilder = false;
    bool inShop       = false;
    bool inOptions    = false;
    bool inPrepare    = false;
    bool inSurvival   = false;

    while (!WindowShouldClose()) {
        WindowManager::GetInstance().Update();
        // Clamp the step. An unbounded dt after a hitch (asset upload, window
        // move, alt-tab) makes everything teleport for one frame, which reads
        // as a much worse stutter than the hitch itself.
        float dt = std::min(GetFrameTime(), MAX_FRAME_DELTA);
        SoundManager::GetInstance().Update(dt);
        AchievementManager::GetInstance().Update(dt);
        Survival3D::SurvivalRunService::GetInstance().Update(dt);

        if (!inGame && !inMapBuilder && !inShop && !inOptions && !inPrepare && !inSurvival) {
            menu.Update(dt);

            if (menu.ShouldOpenShop()) {
                menu.ResetFlags();
                inShop = true;
                shop.Open();
            } else if (menu.ShouldOpenOptions()) {
                menu.ResetFlags();
                inOptions = true;
                opts.SetVisible(true);
            } else if (menu.ShouldStartSurvival()) {
                menu.ResetFlags();
                inSurvival = true;
                survival.Start();
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
        } else if (inSurvival) {
            survival.Update(dt);
            if (survival.ShouldReturnToMenu()) {
                survival.Shutdown();
                inSurvival = false;
                menu.ShowMainMenu();
                continue;
            }

            BeginDrawing();
            ClearBackground(BLACK);
            survival.Render();
            AchievementManager::GetInstance().RenderPopup();
            EndDrawing();
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
    survival.Shutdown();
    MapBuilderController::GetInstance().ExitEditor();

    // Drop view-owned atlases and textures while the OpenGL context is alive.
    View::UIStateManager::GetInstance().Shutdown();
    View::PrepareView::GetInstance().Shutdown();
    shop.Shutdown();
    opts.Shutdown();
    View::InteractPrompt::GetInstance().Shutdown();
    View::SkillBarView::GetInstance().Shutdown();
    View::HUDView::GetInstance().Shutdown();
    View::MapBuilderView::GetInstance().Shutdown();
    View::GameView::GetInstance().Shutdown();
    View::MinimapView::GetInstance().Shutdown();
    Survival3D::SurvivalView::GetInstance().Shutdown();
    Survival3D::SurvivalRunService::GetInstance().Shutdown();
    menu.Shutdown();
    AchievementManager::GetInstance().Shutdown();

    // Asset atlases and audio devices must be gone before the renderer/window.
    assetManager.Shutdown();
    SoundManager::GetInstance().CloseAudio();
    View::Renderer::GetInstance().Shutdown();
    CloseWindow();
    return 0;
}
