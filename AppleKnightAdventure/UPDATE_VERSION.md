# Update Version

## S?a l?i & Tính nang m?i
- Thêm nút **Clear**: Ðã có thêm nút Clear màu cam trên thanh Toolbar d? xóa toàn b? b?n d? nhanh chóng.
- Fix l?i **Entities không d?t du?c/Vô hình**: Th?c t? các entities dã du?c d?t thành công nhung chua có code render (v?) chúng ra trong ch? d? Map Builder. Tôi dã b? sung tính nang v? tr?c ti?p các h?p màu vi?n kèm theo Ch? vi?t t?t (P=Player, E=Enemy, I=Item, C=Chest...) ? ngay trên b?n d? d? b?n có th? nhìn th?y nh?ng entities mình v?a d?t.
- **Thêm lo?i (Type) và hình ?nh cho Entities**: Ðã phân nhánh l?i m?c Entities trong Palette. Bây gi? b?n có d?y d?: Player, Enemy (Melee), Enemy (Ranged), Boss, Ð?ng ti?n (Coin), Qu? táo (Apple), Chìa khóa (Key), Bình máu (Potion).

## Files Modified
- include/View/MapBuilderView.h
- src/View/MapBuilderView.cpp
- src/Controller/MapBuilderController.cpp

## Status
Hoàn t?t các ch?c nang Clear, d?t nhi?u lo?i v?t ph?m/k? thù, và gi? các v?t th? dã hi?n th? rõ ràng trên map khi du?c d?t xu?ng.
