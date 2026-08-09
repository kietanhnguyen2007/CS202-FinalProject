# Update: Implement Fake Wall Collision and Combat
- Added `m_tileX` and `m_tileY` to `FakeWall` to keep track of its tile location.
- Updated `GameController` to process FakeWall correctly for collisions (`IsRectOnGround` and `ResolveTileCollisions`).
- Updated `GameController::UpdateCombat` to process damage applied to `FakeWall` and remove the underlying tile when destroyed.
- Updated `GameView` to draw a pulsating glow on `FakeWall` to indicate it is destructible.
- Bound `GetAllEntities()` from `GameState` to `GameView` every frame in `GameController::Update`.
