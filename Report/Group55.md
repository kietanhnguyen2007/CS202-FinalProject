# Weekly Report

## General Information
- **Group ID:** 55
- **Project Name:** CS202 - Apple Knight
- **GitHub Repository:** https://github.com/kietanhnguyen2007/CS202-FinalProject
- **Date range:** 29/6/2026 - 3/7/2026

## Tasks Completed This Week

**25125074 – Nguyen Anh Kiet**
- Defined and set up the overall MVC (Model-View-Controller) architecture for the game.
- Implemented the core data models (`Player`, `Enemy`, `Boss`, `Item`, etc.) and the `GameState`.
- Developed the `GameController` and `MenuController` to manage game flow and logic.
- Built the networking module (TCP client/server) using an authoritative server model.
- Created the `ElementalSystem` for managing status effects and elemental reactions (Vaporize, Conduct, Overload).
- Evidence: Source code in `include/Model/`, `include/Controller/`, `include/Network/`, and `include/Systems/ElementalSystem.h`.

**25125037 – Nguyen Trong Tien**
- Set up Raylib graphics and implemented the `Renderer` and View layer.
- Designed and integrated UI components (`HUDView`, `MenuView`, `GameView`).
- Implemented the `WindowManager` system for dynamic UI scaling and monitor DPI support.
- Built the `Quadtree` spatial partitioning system to optimize collision detection.
- Developed the `ObjectPool` system to ensure zero runtime allocation during gameplay.
- Implemented the `ParticleSystem` for visual effects.
- Evidence: Source code in `include/View/`, `include/Systems/WindowManager.h`, `include/Systems/Quadtree.h`, and `include/Systems/ObjectPool.h`.

## AI Usage Declaration
- **Using to fix error:** AI was used to debug and fix memory allocation issues within the main game loop, ensuring the strict zero runtime allocation constraints were met. It also assisted in fixing C++ compilation errors.
- **Suggest some idea and logic for project:** AI suggested the implementation of the Object Pool pattern for managing projectiles and particles. It also provided the architectural blueprint for the dynamic UI scaling logic and `WindowManager` integration.

## Tasks Planned for Next Week
- Polish and test multiplayer network synchronization to ensure smooth movement prediction.
- Implement more level designs and integrate them seamlessly with the `LevelFactory`.
- Add comprehensive sound effects and background music using the `SoundManager`.
- Finalize boss AI patterns and gameplay balancing for co-op mode.

## Issues
- **Issue:** The game experienced lag spikes during combat when many projectiles and particles were spawned, due to dynamic memory allocation.
  - **Resolution:** Addressed this by implementing an `ObjectPool` to pre-allocate and reuse entities, which completely removed runtime memory allocation overhead.
- **Issue:** The UI became distorted or misaligned when the window was resized or moved to a monitor with a different DPI scale.
  - **Resolution:** Created a `WindowManager` singleton to automatically detect window resizing and recalculate the DPI scale, keeping the UI perfectly proportioned using letterbox-safe scaling.
