#ifndef INPUTCONTROLLER_H
#define INPUTCONTROLLER_H

struct InputCommand {
    bool moveLeft      = false;
    bool moveRight     = false;
    bool jump          = false;
    bool attack        = false;  // J — Attack1 (quick slash)
    bool parry         = false;  // K — Attack2 (heavy strike)
    bool skill1        = false;  // U — Attack3 (lunge thrust)
    bool skill2        = false;  // (reserved)
    bool ultimate      = false;  // R
    bool interact      = false;  // F or O
    bool openInventory = false;  // I
    bool pause         = false;  // ESC
    int  menuDelta     = 0;
    bool menuConfirm   = false;
    // Movement modifiers
    bool sprint        = false;  // SHIFT — hold to run faster
    bool dash          = false;  // L — tap to dash
    // Pet
    bool pet1          = false;  // 1 — summon Dragon
    bool pet2          = false;  // 2 — summon Ghost
};

class InputController {
public:
    static InputController& GetInstance();

    InputCommand Poll();

private:
    InputController() = default;
};

#endif
