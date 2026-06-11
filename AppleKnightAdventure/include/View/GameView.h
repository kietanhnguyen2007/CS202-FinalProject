#pragma once

namespace View {

class GameView {
public:
    static GameView& GetInstance();

    void Init();
    void Update(float dt);
    void Render();
    void Shutdown();

private:
    GameView() = default;
    ~GameView() = default;
};

} // namespace View
