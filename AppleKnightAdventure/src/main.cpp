#include "raylib.h"
#include "Utils/Constants.h"
#include "Controller/GameController.h"

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Apple Knight Adventure");
    SetTargetFPS(60);

    Controller::GameController controller;
    if (!controller.Init()) {
        CloseWindow();
        return -1;
    }

    while (!WindowShouldClose()) {
        controller.Update();
        controller.Render();
    }

    controller.Shutdown();
    CloseWindow();

    return 0;
}
