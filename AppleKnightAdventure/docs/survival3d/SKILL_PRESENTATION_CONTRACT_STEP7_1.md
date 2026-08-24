# Step 7.1 — Skill presentation contract

Nguồn dữ liệu production nằm tại `assets/survival3d/config/skill_presentation_manifest_v2.json`.

## Mục đích

Contract khóa đầy đủ chuỗi trình diễn cho năm skill Knight và năm skill Magic Caster:

`animation → main shape → particle/trail/glow/distortion → impact → light/sound/camera shake → dissolve`.

Ảnh trong `assets/survival3d/concepts/skills/final_v2` là source ImageGen quyết định silhouette, palette, pattern, rune và vật liệu của **main shape**. Ảnh không được vẽ trực tiếp như một skill tĩnh trong game. `runtimeModel` là mesh 3D tương ứng; Blender/C++ tạo các lớp chuyển động và presentation còn lại.

## Quy tắc tích hợp

- `characterAction`, `heroTrackId`, `eventId`, `frameCount` và mọi event có `channel: gameplay` khớp `AnimationEvents.cpp`; không được dịch frame contact để làm VFX trông đẹp hơn.
- Event `channel: presentation` chỉ tạo hình ảnh, âm thanh hoặc camera response. Nó không tự gây damage.
- Event dùng `trigger` thay vì `frame` được phát từ gameplay runtime, ví dụ `projectile_contact` và `parry_success`.
- Main shape, emitter, ribbon, light và impact instance phải lấy từ `poolClasses`; không cấp phát heap trong combat hot path.
- `performanceBudget` là mức tối đa cho **một instance** ở quality High/1080p. Khi quá tải, giảm cosmetic particle trước, sau đó trail segment; không bỏ enemy telegraph hoặc gameplay hit feedback.
- Reduced Motion dùng các hệ số tại `globalAccessibility`, nhưng vẫn giữ flash contact, main shape và âm thanh gameplay quan trọng.

## Chia trách nhiệm asset

| Lớp | Nguồn |
|---|---|
| Main shape, palette, rune, crystal/metal design | ImageGen reference → mesh 3D |
| Mesh cleanup, animation `Activate`, socket và material | Blender |
| Particle simulation, ribbon trail, glow, distortion và dynamic light | C++/shader dựa trên contract |
| Damage, hitbox, projectile release và parry result | Animation event/gameplay runtime |
| Cast, travel, impact audio và camera shake | Presentation runtime |

## Cổng nghiệm thu Step 7.1

- Có đúng 10 skill, chia đều Knight/Magic Caster.
- Mỗi skill khai báo đủ phase, event, main shape, particle, trail, glow, distortion, impact, light, sound, camera shake, budget và pooling class.
- Mọi concept ImageGen và GLB được tham chiếu bằng đường dẫn ổn định, không dùng ảnh màn hình tĩnh.
- Projectile impact và parry impact dùng external trigger, tránh giả định contact tại một frame cố định.
- Ultimate có budget riêng và cosmetic pulse không tạo thêm damage ngoài event gameplay.
