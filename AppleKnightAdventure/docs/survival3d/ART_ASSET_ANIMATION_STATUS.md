# Aegis Rift — Art asset & animation status

## Production concept assets

Các sheet dưới đây được tạo bằng built-in ImageGen từ thiết kế 2D hiện có và spec GDD. Chúng khóa silhouette, palette, material và tỷ lệ trước khi chuyển sang skeletal model:

| Asset | Phạm vi |
|---|---|
| `knight_3d_turnaround_v1.png` | Knight front/side/back, purple armor + greatsword |
| `magic_caster_3d_turnaround_v1.png` | Caster front/side/back, blue cape + crystal staff |
| `core_enemies_3d_lineup_v1.png` | Riftling, Hex Archer, Obsidian Brute |
| `bosses_wave10_30_3d_lineup_v1.png` | Brood Warden, Hexeye Artillerist, Ironroot Colossus |
| `bosses_wave40_50_3d_lineup_v1.png` | Eclipse Chimera và ba phase Void Sovereign |

Runtime không đọc các PNG concept như sprite. Chúng là source-of-truth hình ảnh cho các model Blender thật. Art pass hiện tại gồm 10 source `.blend` và 10 GLB low-poly đã retopo theo hướng game-ready, rig 15 bone, PBR material nhúng và skeletal animation. Game chỉ tải GLB bằng C++/raylib; không có Python runtime.

## 3D production pipeline đã áp dụng

1. Modeling theo turnaround; mỗi source `.blend` giữ cả `*_High` 7,7k–9,6k vertex để sculpt/bake và `*_Low` 508–624 vertex cho game.
2. Retopology/game topology: một skinned Low mesh/model, 4–5 material primitives, 15 bone; phụ kiện cứng weight 100% vào bone tương ứng để deformation ổn định.
3. Texturing/material: Principled PBR, palette riêng, metallic/roughness và emissive core nhúng trong GLB. Source `.blend` là điểm vào để thay bằng texture Substance Painter ở art pass chi tiết mà không đổi contract runtime.
4. Rigging: shared hierarchy gồm root, hips, spine, head, arm/leg hai bên, weapon và hai special bones.
5. Animation: một sampled action set 655 frame/60 FPS; C++ chia 9 clip và cross-fade 0,14 giây.
6. Export: GLB có skin/material/animation, một mesh để giảm draw-call; CMake tự đồng bộ sang runtime assets.

## Animation states đã tích hợp cho toàn bộ roster

| State | Knight | Magic Caster |
|---|---|---|
| Idle | breathing + ponytail | breathing + staff crystal pulse |
| Run | leg/arm counter-swing + bob | leg/arm counter-swing + cape body motion |
| BasicAttack | greatsword sweep | staff cast/recoil |
| SkillOne | guard raise | frost-control cast |
| SkillTwo | shield rush lean | gravity-well cast |
| Ultimate | overhead greatsword charge | staff raise + hand aura |
| Dash | forward lean | blink lean |
| Hurt | recoil | recoil |
| Death | body fall pose | body fall pose |

Ba enemy và năm boss cũng có đủ chín đoạn tương ứng; với boss, `Ultimate` là phase transition và `Dash` là special/summon. `F5 Reduced Motion` chỉ giảm camera/bob, không đóng băng skeletal animation.

## Runtime behavior

- `LoadModel` + `LoadModelAnimations` kiểm tra skeleton khi khởi động; procedural C++ renderer cũ chỉ là fallback nếu GLB thiếu/hỏng.
- `UpdateModelAnimationEx` blend pose cũ/mới trong 0,14 giây, tránh snap khi idle → run → attack/hurt.
- Enemy death giữ entity trong pool 0,95 giây; boss 1,55 giây, đủ phát clip death rồi mới release.
- LOD gần dùng skeletal model đầy đủ; ngoài 21 m enemy thường chuyển sang silhouette proxy có pulse nhẹ và AI 30 Hz. Boss luôn giữ model/animation đầy đủ.
- Frame map chuẩn nằm ở `assets/survival3d/config/animation_manifest.json`.

## ImageGen prompt set

- Hero turnaround: chuyển đúng identity/costume/palette từ sprite 2D sang ba góc orthographic low-poly, neutral studio, không chữ và không redesign.
- Core enemy lineup: ba silhouette chiến thuật Swarm/Ranger/Tanker, palette violet-orange/green/charcoal-gold, topology-friendly.
- Boss roster 10–30: insect queen, floating rune eye và tree-stone colossus với signature core/mechanic parts.
- Boss roster 40–50: dual-core Solar/Lunar chimera và ba form Void Sovereign tăng dần 2.4/4.8/7.5 m.
