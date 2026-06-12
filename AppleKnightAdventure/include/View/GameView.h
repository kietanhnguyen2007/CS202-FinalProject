#pragma once

#include "raylib.h"
#include <vector>

struct Particle;

namespace View {

class GameView {
public:
    static GameView& GetInstance();

    void Init();
    void Update(float dt);
    void Render(const Camera2D& camera, const std::vector<Particle*>& particles = {});
    void Shutdown();

private:
    GameView() = default;
    ~GameView() = default;
};

} // namespace View
