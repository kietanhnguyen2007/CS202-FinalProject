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

    // Load game_font for loading screen if available
    Font loadingFont = LoadFont("assets/fonts/game_font.ttf");
    bool hasFontLoaded = (loadingFont.texture.id != 0);
    float loadingTime = 0.0f;

    while (!WindowShouldClose() && !assetManager.IsLoadingComplete()) {
        assetManager.UpdateMainThread();
        float dt = GetFrameTime();
        loadingTime += dt;

        float displayProg = assetManager.GetDisplayProgress();
        float realProg    = assetManager.GetProgress();
        int   percent     = static_cast<int>(realProg * 100.0f);

        // Current asset name
        std::string assetName = assetManager.GetCurrentAssetName();
        if (assetName.empty()) assetName = "Initializing...";
        else assetName = "Loading: " + assetName;

        BeginDrawing();
        ClearBackground(Color{10, 8, 20, 255});

        int sw = SCREEN_WIDTH;
        int sh = SCREEN_HEIGHT;

        // ── Dark radial vignette
        for (int r = sh; r > 0; r -= 20) {
            unsigned char alpha = static_cast<unsigned char>(80.0f * (1.0f - (float)r / sh));
            DrawCircle(sw/2, sh/2, (float)r, Color{0, 0, 0, alpha});
        }

        // ── Title
        const char* title = "Apple Knight Adventure";
        int titleSize = 52;
        int titleW = MeasureText(title, titleSize);
        DrawText(title, sw/2 - titleW/2 + 2, sh/2 - 160 + 2, titleSize, Color{0,0,0,120}); // shadow
        DrawText(title, sw/2 - titleW/2,     sh/2 - 160,     titleSize, Color{255,230,80,255});

        // ── Progress bar (rounded look with layered rects)
        int barW = 480, barH = 18;
        int barX = sw/2 - barW/2;
        int barY = sh/2 - 10;
        DrawRectangle(barX - 2, barY - 2, barW + 4, barH + 4, Color{60,50,80,200});  // border
        DrawRectangle(barX, barY, barW, barH, Color{20,15,35,255});                   // bg
        int fillW = static_cast<int>(barW * displayProg);
        if (fillW > 0) {
            // Gradient: dark purple -> bright gold
            DrawRectangleGradientH(barX, barY, fillW, barH,
                Color{120, 60, 200, 255}, Color{255, 200, 40, 255});
        }
        // Shimmer effect on fill edge
        if (fillW > 4) {
            float shimmer = (sinf(loadingTime * 6.0f) + 1.0f) * 0.5f;
            unsigned char sa = static_cast<unsigned char>(100 + shimmer * 120);
            DrawRectangle(barX + fillW - 4, barY, 4, barH, Color{255,255,255,sa});
        }

        // ── Percent text
        char pctBuf[16];
        snprintf(pctBuf, sizeof(pctBuf), "%d%%", percent);
        int pctW = MeasureText(pctBuf, 28);
        DrawText(pctBuf, sw/2 - pctW/2, barY + barH + 12, 28, Color{220,200,255,230});

        // ── Asset name (current file)
        int nameSize = 18;
        int nameW = MeasureText(assetName.c_str(), nameSize);
        DrawText(assetName.c_str(), sw/2 - nameW/2, barY - 36, nameSize, Color{160,145,200,200});

        // ── Dot spinner bottom
        float spinAngle = loadingTime * 180.0f; // degrees per second
        for (int d = 0; d < 6; d++) {
            float angle = (spinAngle + d * 60.0f) * DEG2RAD;
            float r2 = 20.0f;
            float dx = cosf(angle) * r2;
            float dy = sinf(angle) * r2;
            float age = fmodf(loadingTime + d * 0.11f, 0.6f) / 0.6f;
            unsigned char da = static_cast<unsigned char>(255 * (1.0f - age));
            DrawCircle((int)(sw/2 + dx), sh/2 + 90 + (int)dy, 4.0f, Color{200,160,255,da});
        }

        EndDrawing();
    }
    if (hasFontLoaded) UnloadFont(loadingFont);
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
