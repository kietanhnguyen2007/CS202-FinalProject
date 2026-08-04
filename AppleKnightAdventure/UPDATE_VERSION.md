# UPDATE_VERSION

## Cập nhật nhánh Map Builder (Tích hợp tính năng & UI)
- Thêm nút **Clear**: Đã có thêm nút Clear màu cam trên thanh Toolbar để xóa toàn bộ bản đồ nhanh chóng.
- Fix lỗi **Entities không đặt được/Vô hình**: Thực tế các entities đã được đặt thành công nhưng chưa có code render (vẽ) chúng ra trong chế độ Map Builder. Tôi đã bổ sung tính năng vẽ trực tiếp các hộp màu viền kèm theo Chữ viết tắt (P=Player, E=Enemy, I=Item, C=Chest...) ở ngay trên bản đồ để bạn có thể nhìn thấy những entities mình vừa đặt.
- **Thêm loại (Type) và hình ảnh cho Entities**: Đã phân nhánh lại mục Entities trong Palette. Bây giờ bạn có đầy đủ: Player, Enemy (Melee), Enemy (Ranged), Boss, Đồng tiền (Coin), Quả táo (Apple), Chìa khóa (Key), Bình máu (Potion).
- **Fix Quit Test Button, Crash & Visibility**: Fix lỗi văng game khi ấn nút Quit Test từ Playtest mode (bằng cách xóa bộ đệm vẽ CharacterRenderer) và sửa lỗi hiển thị sót nút Quit Test trong màn hình Map Builder.

## Cập nhật nhánh Tien (Tính năng Boss Arena)
### Modified Files
- include/Model/TeleportPortal.h - Added BossArena to PortalType enum; added IsLocked(), SetLocked() and m_isLocked
- src/Model/TeleportPortal.cpp - Implement lock/unlock; CanInteract() returns false when locked
- include/Controller/GameController.h - Added PlayerSaveState struct, boss arena fields (m_previousLevelId, m_exitSpawnPos, m_savedPlayerState, m_hasSavedState), and helpers
- src/Controller/GameController.cpp - StartLevel() preserves CharacterClass; UpdateInteractions() handles BossArena portal; helpers UpdateBossArenaPortals, SavePlayerState, RestorePlayerState added; Update() calls UpdateBossArenaPortals
- include/Factories/LevelFactory.h - Added CharacterClass cls param to LoadLevel() and LoadLDtkLevel()
- src/Factories/LevelFactory.cpp - Spawn points use Player(pos, cls); portal type parsing handles BossArena

### What Changed and Why
**Boss Arena Portal** - New PortalType::BossArena. Entry portal saves player state (HP/score/coins/apples/keys) and records exit spawn position. Exit portal auto-locked until all enemies/bosses dead. On exit, state restored and player spawns at exit position (beside the original entry portal).

**Exit Spawn Point** - No new LDtk entity. Position is right-edge of entry portal bounding box, stored in m_exitSpawnPos before level transition.

**Player State Preservation** - PlayerSaveState snapshot. Saved on enter, restored on return. Boss rewards captured before returning.

**SkillSet Init Fix** - LevelFactory passes cls to Player(pos, cls) so the correct SkillSet (Knight/Fighter/Ninja/MagicCaster) is initialized from spawn.

## LDtk Map Builder Instructions
To create a boss arena connection:
1. **Level main** - place Portal entity: PortalType=BossArena, TargetLevelId=<boss_level_index>
2. **Level boss** - place Portal entity: PortalType=BossArena, TargetLevelId=-1 (exit), do NOT place CheckpointEnd
3. Player enters boss arena with current HP/score/inventory preserved
4. After killing all enemies, exit portal unlocks automatically

## Current Status
Build: PASS (no errors, no warnings)
Tất cả các tính năng của Map Builder (bao gồm giao diện, hiển thị Entity) và Boss Arena của nhánh Tien đã được tích hợp thành công.
