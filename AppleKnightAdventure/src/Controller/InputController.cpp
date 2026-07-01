#include "Controller/InputController.h"
#include "raylib.h"

InputController& InputController::GetInstance() {
    static InputController instance;
    return instance;
}

InputCommand InputController::Poll() {
    InputCommand cmd;

    cmd.moveLeft = IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT);
    cmd.moveRight = IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT);
    cmd.jump = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP);
    cmd.attack = IsKeyPressed(KEY_J) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    cmd.parry = IsKeyPressed(KEY_K);
    cmd.skill1 = IsKeyPressed(KEY_Q);
    cmd.skill2 = IsKeyPressed(KEY_E);
    cmd.ultimate = IsKeyPressed(KEY_R);
    cmd.interact = IsKeyPressed(KEY_F);
    cmd.openInventory = IsKeyPressed(KEY_I);
    cmd.pause = IsKeyPressed(KEY_ESCAPE);

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) cmd.menuDelta = -1;
    else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) cmd.menuDelta = 1;
    cmd.menuConfirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);

    return cmd;
}
