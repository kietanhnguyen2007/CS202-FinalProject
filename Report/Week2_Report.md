# Weekly Report (Week 2)

## General Information
- **Group ID:** 55
- **Project Name:** CS202 - Apple Knight
- **GitHub Repository:** https://github.com/kietanhnguyen2007/CS202-FinalProject
- **Date range:** 3/7/2026 - 10/7/2026

## Tasks Completed This Week

**25125074 – Nguyen Anh Kiet**
- **Audio Assets Research & Documentation:** Addressed the missing task from last week by thoroughly researching and compiling `SOUND_ASSET_GUIDE.md` and `DOWNLOAD_SOUND_ASSETS.md`. These documents map all game states, character classes, UI events, and elemental reactions to specific, verified free audio assets (Kenney, OpenGameArt).
- **Dynamic Sound Loading System:** Refactored `GameController::Init()` and implemented `LoadPlayerSounds(CharacterClass)` to dynamically load class-specific audio (Fighter, Knight, MagicCaster, Ninja) into generic internal keys (e.g., `player_attack1`), eliminating hardcoded logic inside the combat loops.
- **Backend & UI Event Hookups:** Successfully integrated sound triggers into the core engine without altering existing physics logic. Hooked up Player inputs, Enemy/Boss states, Pet actions (summon, dragon fire, ghost heal), and `ElementalSystem` reactions (Vaporize, Conduct, Overload). Also integrated UI sound triggers for game states in `ResultView` and `MenuController`.
- **Repository Optimization & Build System:** Removed multiple heavy, unused dummy `.wav` files to reduce project footprint. Updated `CMakeLists.txt` and `.gitignore` to manage audio assets effectively.

**25125037 – Nguyen Trong Tien**
- **Enemy Combat Animations:** Developed and integrated attack animations for enemy entities within the main combat loop, ensuring smooth visual feedback during monster strikes.
- **End Game Checkpoint Flag:** Implemented the final checkpoint logic and interaction flags (`Item.h`, `Types.h`, `GameController.cpp`) to successfully trigger the end game sequence when the player reaches the goal.
- **Demo Game Polish & Interaction (carried over from late Week 1):** Finalized the playable demo level rendering, Knight combat combos (J, K, U keys), and Pet support constraints (Dragon assist, Ghost heal range).

## AI Usage Declaration
- **Using to fix error:** AI was used to verify that inserting `PlaySound` calls into the high-frequency fixed-timestep loops (like `UpdateCombat` and `HandlePlayerInput`) did not introduce dynamic allocation or violate the strict Zero-Allocation rule.
- **Suggest some idea and logic for project:** AI suggested the strategy of using dynamic search queries for OpenGameArt and Freesound instead of static links in the Asset Guide to completely eliminate the risk of 404 errors. It also suggested mapping class-specific audio to generic keys to keep `UpdatePlayerInput` clean.

## Tasks Planned for Next Week
- Finalize downloading and importing all actual `.wav` and `.ogg` files based on the Asset Guide.
- Thoroughly test the audio system across the TCP network (ensuring Host and Client hear synchronized combat sounds).
- Continue refining boss AI patterns and dual-world level switching mechanics.
- Prepare the final project presentation and demo video.

## Issues
- **Issue:** Finding high-quality, free, and fitting sound assets for 4 distinct player classes and elemental reactions was highly time-consuming and prone to broken web links.
  - **Resolution:** Used reliable open-source platforms (Kenney.nl) and created a dynamic search link strategy (`DOWNLOAD_SOUND_ASSETS.md`) to guarantee asset availability without 404 errors.
- **Issue:** The repository size was ballooning due to large, unused dummy `.wav` files.
  - **Resolution:** Purged the unnecessary files and updated `.gitignore` to manage audio assets more effectively outside of direct source control tracking.
