# Cập nhật Khả năng Tấn công của Quái vật

## Thay đổi
1. `include/Utils/Constants.h`: 
   - Tăng `ENEMY_MELEE_RANGE` từ `40` lên `100` (ngang bằng với vùng di chuyển `m_patrolRange`). Trước đây tầm đánh quá ngắn khiến nhân vật và quái vật bị cản lại bởi hitbox vật lý và không bao giờ chạm tới được ngưỡng 40 pixels, khiến quái vật không bao giờ kích hoạt được trạng thái `Attack`.

2. `src/Model/Enemy.cpp`:
   - **Đứng lại để tấn công**: Thêm lệnh ép vận tốc ngang về 0 (`MoveX(0, deltaTime)`) khi quái vật rơi vào trạng thái `EnemyState::Attack` hoặc `EnemyState::Hurt`. Nhờ đó, quái vật sẽ không bị trượt (slide) về phía trước khi đang phát animation vung vũ khí hoặc bị giật lùi, mang lại cảm giác chân thực hơn.
   - **Sửa lỗi animation tuần tra**: Chuyển mapping trạng thái từ `EnemyState::Patrol` sang `Character::State::Walk` (trước đó là `Idle`), giúp quái vật hiện đúng hoạt ảnh "bước đi" khi đang đi tuần, thay vì tư thế đứng im trượt trên mặt đất.

3. `src/Controller/GameController.cpp`:
   - Hệ thống gây sát thương tự động (`UpdateCombat`) giờ đã hoạt động chính xác vì quái vật có thể tiến vào trạng thái `Attack` đúng cách và kích hoạt đòn đánh lên Player khi khoảng cách hợp lệ.

## Trạng thái hiện tại
- Quái vật sẽ tự động lao tới và ra đòn (vung kiếm/ném bom) khi Player lọt vào vùng di chuyển (patrol range) của nó.
- Có đầy đủ animation Attack (đã load từ các bước trước) và quái vật sẽ đứng tấn vững vàng khi vung vũ khí.
- Trải nghiệm chiến đấu đã trở nên hoàn thiện hơn.
