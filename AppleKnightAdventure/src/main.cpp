#include "Controller/MenuController.h"
#include "Controller/GameController.h"
#include "View/Renderer.h"
#include "View/MenuView.h"
#include "Utils/Constants.h"
#include "raylib.h"

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Apple Knight Adventure");
    SetTargetFPS(60);

    if (!View::Renderer::GetInstance().Init()) {
        CloseWindow();
        return 1;
    }

    auto& menu = MenuController::GetInstance();
    auto& game = GameController::GetInstance();
    menu.Init();

    bool inGame = false;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (!inGame) {
            menu.Update(dt);
            if (menu.ShouldStartGame()) {
                inGame = true;
                game.Init();
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
    View::Renderer::GetInstance().Shutdown();
    CloseWindow();
    return 0;
}
