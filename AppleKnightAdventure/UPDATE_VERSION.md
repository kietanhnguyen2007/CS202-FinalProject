# UPDATE_VERSION — Background Theme Refactor

## Branch
`feature/background-theme-refactor` (branched from `Tien`)

## Files Created / Modified

### Assets (12 files)
- DELETED: `assets/textures/backgrounds/forest/` (7 Anokolisa layers)
- NEW forest: back.png, middle.png, front.png (Ansimuz Parallax Forest v2)
- NEW cold_corridor: back.png, far.png, middle.png, near.png, foreground.png (Gothicvania Cold Corridors)
- NEW underwater: far.png, foreground_1.png, foreground_2.png, sand.png (Underwater Fantasy)

### Code
- `include/Model/GameState.h` — Thêm enum BackgroundTheme, field + getter/setter
- `src/Model/GameState.cpp` — Implement getter/setter
- `include/View/GameView.h` — Đổi signature LoadBackgrounds(BackgroundTheme theme = Forest)
- `src/View/GameView.cpp` — Xoá hardcode 7 layer, thay bằng static lookup table
- `src/Controller/GameController.cpp` — Comment TODO cho map builder

## What Changed & Why
Root cause: LoadBackgrounds() hardcode 7 path Anokolisa, View tự quyet data — vi pham MVC.
Fix: BackgroundTheme enum trong Model, View nhan theme qua param, dung lookup table.
Assets: Anokolisa -> 3 pack Ansimuz CC0 (Forest v2, Cold Corridors, Underwater Fantasy).

## Status
- Code compile-ready (backward compatible — default param Forest)
- Assets dung cho
- Chua co mapping level -> theme (cho map builder)
