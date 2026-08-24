# Aegis Rift — Design Package

Tài liệu thiết kế cho minigame 3D Wave Survival Roguelite của Apple Knight Adventure:

- [GDD — gameplay, art, character, enemy, 50 wave, boss và roguelite](./GDD_3D_WAVE_SURVIVAL.md)
- [TDD — client C++/raylib, performance, backend, database và API](./TDD_3D_WAVE_SURVIVAL.md)

Baseline này chủ động giữ minigame trong cùng executable với campaign 2D, nhưng tách runtime entity/collision/render 3D thành module riêng. Hệ thống dùng chung gồm menu, input, options, audio, font và save.

## Implementation status — `codex/survival3d`

- M1: hoàn thành arena/camera, Knight, combat và mode router từ Main Menu.
- M2 foundation: fixed timestep, Wave Director, object pools, HUD và upgrade 1-trong-3.
- M3 content alpha: Magic Caster, Riftling, Hex Archer, Obsidian Brute, Brood Warden (Wave 10) và Hexeye Artillerist (Wave 20).
- M4 50-wave beta: năm boss, Void Sovereign ba phase, rarity/stack/pity upgrade pool, shield/revive/element synergy và JSON balance cho đủ 50 wave.
- M5 services alpha: local profile/top-5 history trong `save.json`, reward idempotent, offline submission queue/backoff, Rift Records UI và C++17 JSON dev service có validation + leaderboard.
- M6 stabilization: WinHTTP worker hai chiều (submit + Global Leaderboard) nối REST server Winsock C++, guest identity + queue sync, far-enemy AI/render LOD, F3 profiler, F4 high contrast, F5 reduced motion, F6 UI scale, JSONL balance telemetry và automated config QA.
- Art pass B: 10 model Blender/GLB thật cho 2 hero, 3 enemy và 5 boss; shared 15-bone rig, 655 sampled frame/60 FPS, đủ 9 action/state, C++ cross-fade và death linger. Procedural renderer chỉ còn là fallback an toàn.
