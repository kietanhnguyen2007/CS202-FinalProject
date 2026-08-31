#include "Controller/InputController.h"
#include "raylib.h"

InputController& InputController::GetInstance() {
    static InputController instance;
    return instance;
}

InputCommand InputController::Poll() {
    InputCommand cmd;

    cmd.moveLeft  = IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT);
    cmd.moveRight = IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT);
    cmd.jump      = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP);
    cmd.attack    = IsKeyPressed(KEY_J) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT); // Attack1
    cmd.parry     = IsKeyPressed(KEY_K);  // Attack2 (heavy)
    cmd.skill1    = IsKeyPressed(KEY_U);  // Attack3 (lunge)
    cmd.ultimate  = IsKeyPressed(KEY_H);
    cmd.parryBlock = IsKeyDown(KEY_P);  // Hold P to keep parrying
    cmd.interact  = IsKeyPressed(KEY_F) || IsKeyPressed(KEY_O);
    cmd.pause     = IsKeyPressed(KEY_ESCAPE);
    cmd.sprint    = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    cmd.dash      = IsKeyPressed(KEY_L);

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) cmd.menuDelta = -1;
    else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) cmd.menuDelta = 1;

    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) cmd.menuDeltaX = -1;
    else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) cmd.menuDeltaX = 1;

    cmd.menuConfirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);

    return cmd;
}

InputCommand InputController::PollPlayerOne() {
    InputCommand cmd;
    cmd.moveLeft   = IsKeyDown(KEY_A);
    cmd.moveRight  = IsKeyDown(KEY_D);
    cmd.jump       = IsKeyPressed(KEY_W) || IsKeyPressed(KEY_SPACE);
    cmd.attack     = IsKeyPressed(KEY_J);
    cmd.parry      = IsKeyPressed(KEY_K);
    cmd.skill1     = IsKeyPressed(KEY_U);
    cmd.ultimate   = IsKeyPressed(KEY_H);
    cmd.parryBlock = IsKeyDown(KEY_P);
    cmd.interact   = IsKeyPressed(KEY_F);
    cmd.pause      = IsKeyPressed(KEY_ESCAPE);
    cmd.sprint     = IsKeyDown(KEY_LEFT_SHIFT);
    cmd.dash       = IsKeyPressed(KEY_L);
    return cmd;
}

InputCommand InputController::PollPlayerTwo() {
    InputCommand cmd;
    cmd.moveLeft   = IsKeyDown(KEY_LEFT);
    cmd.moveRight  = IsKeyDown(KEY_RIGHT);
    cmd.jump       = IsKeyPressed(KEY_UP);
    cmd.attack     = IsKeyPressed(KEY_KP_1);
    cmd.parry      = IsKeyPressed(KEY_KP_2);
    cmd.skill1     = IsKeyPressed(KEY_KP_3);
    cmd.ultimate   = IsKeyPressed(KEY_KP_0);
    cmd.dash       = IsKeyPressed(KEY_KP_4);
    cmd.parryBlock = IsKeyDown(KEY_KP_5);
    cmd.interact   = IsKeyPressed(KEY_KP_6);
    cmd.sprint     = IsKeyDown(KEY_KP_7);
    return cmd;
}
