#ifndef INPUTCONTROLLER_H
#define INPUTCONTROLLER_H

struct InputCommand {
    bool moveLeft = false;
    bool moveRight = false;
    bool jump = false;
    bool attack = false;
    bool parry = false;
    bool skill1 = false;
    bool skill2 = false;
    bool ultimate = false;
    bool interact = false;
    bool openInventory = false;
    bool pause = false;
    int menuDelta = 0;
    bool menuConfirm = false;
};

class InputController {
public:
    static InputController& GetInstance();

    InputCommand Poll();

private:
    InputController() = default;
};

#endif
