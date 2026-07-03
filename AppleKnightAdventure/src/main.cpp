#include "Controller/MenuController.h"
#include "Controller/GameController.h"
#include "View/Renderer.h"
#include "View/MenuView.h"
#include "View/GameView.h"
#include "View/HUDView.h"
#include "View/AssetManager.h"
#include "Utils/Constants.h"
#include "Systems/WindowManager.h"
#include "raylib.h"
#include <filesystem>
#include <vector>
#include <string>

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Apple Knight Adventure");
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

    while (!WindowShouldClose() && !assetManager.IsLoadingComplete()) {
        assetManager.UpdateMainThread();

        BeginDrawing();
        ClearBackground(BLACK);
        float progress = assetManager.GetProgress();
        int percent = static_cast<int>(progress * 100.0f);
        
        const char* text = TextFormat("Loading... %d%%", percent);
        int textWidth = MeasureText(text, 40);
        DrawText(text, SCREEN_WIDTH / 2 - textWidth / 2, SCREEN_HEIGHT / 2 - 20, 40, WHITE);
        
        // Progress bar
        DrawRectangle(SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT / 2 + 30, 400, 20, DARKGRAY);
        DrawRectangle(SCREEN_WIDTH / 2 - 200, SCREEN_HEIGHT / 2 + 30, (int)(400 * progress), 20, GREEN);
        
        EndDrawing();
    }
    // ---------------------------------------

    auto& menu = MenuController::GetInstance();
    auto& game = GameController::GetInstance();
    menu.Init();
    game.Init();

    bool inGame = false;

    while (!WindowShouldClose()) {
        WindowManager::GetInstance().Update();
        float dt = GetFrameTime();

        if (!inGame) {
            menu.Update(dt);
            if (menu.ShouldStartGame()) {
                inGame = true;
                game.StartLevel(1);
            }
            if (menu.ShouldQuit()) {
                break;
            }

            BeginDrawing();
            ClearBackground(BLACK);
            View::Renderer::GetInstance().BeginFrame();
            View::MenuView::GetInstance().Update(dt, menu.GetSelected());
            View::MenuView::GetInstance().Render();
            View::Renderer::GetInstance().EndFrameAndFlush();
            EndDrawing();
        } else {
            game.Update(dt);
            if (game.ShouldReturnToMenu()) {
                game.Shutdown();
                inGame = false;
                menu.ShowMainMenu();
                
                // Render one frame of menu to consume the input and pump events
                BeginDrawing();
                ClearBackground(BLACK);
                View::Renderer::GetInstance().BeginFrame();
                View::MenuView::GetInstance().Update(dt, menu.GetSelected());
                View::MenuView::GetInstance().Render();
                View::Renderer::GetInstance().EndFrameAndFlush();
                EndDrawing();
                
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
