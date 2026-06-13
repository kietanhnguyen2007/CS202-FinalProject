# Update Version: View & Asset Fixes

## Files Modified:
- `src/View/SkillBarView.cpp`: Fixed compilation error caused by incorrectly assigning a `std::unique_ptr` to a `std::shared_ptr`. Moved the `std::unique_ptr` properly.
- `src/View/GameView.cpp`: Fixed compilation error related to const-correctness when submitting a sprite to the Renderer. Passed `const_cast<Texture2D*>(&layer.tex)` because `SubmitSprite` expects a non-const pointer while `layer` is a const reference.

## Current Status:
- The game now successfully compiles and links without errors.
- Checked asset json references in `SkillBarView.cpp` and confirmed they exist in the `assets` directory.
- No remaining `TODO` or `FIXME` items found in the `View` codebase.
