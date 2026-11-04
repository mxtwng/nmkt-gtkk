# Thiết kế phần mềm điều khiển

## Mục tiêu

Tài liệu này chuyển lưu đồ điều khiển thành cấu trúc để bắt đầu viết chương trình ESP32. Đây là bản thiết kế trung lập với nền tảng: chưa chọn Arduino hay ESP-IDF, chưa gán chân GPIO và chưa chọn thư viện cảm biến. Chỉ chốt các mục đó sau khi xác nhận đúng bo mạch và cách đấu dây.

## Cấu trúc chương trình đề xuất

| Thành phần | Nhiệm vụ |
|---|---|
| `CauHinh` | Khai báo chân GPIO, ngưỡng nhiệt độ và các khoảng thời gian đã được xác nhận |
| `CamBien` | Khởi tạo và đọc DS18B20, SHT31, AHT20, TCRT5000; trả về dữ liệu cùng trạng thái hợp lệ |
| `DauRa` | Điều khiển hai quạt và hai thanh PTC; luôn khởi động ở trạng thái tắt |
| `BoDieuKhien` | Quản lý trạng thái máy, liên động quạt/thanh nhiệt, giới hạn nhiệt và điều kiện dừng |
| `GhiNhan` | Ghi thời gian, nhiệt độ, độ ẩm, trạng thái máy và mã lỗi qua Serial trong giai đoạn thử nghiệm |
| `main` | Khởi tạo các thành phần, gọi vòng lặp điều khiển định kỳ và xử lý lệnh reset lỗi |

Có thể bắt đầu bằng một tệp chương trình duy nhất để kiểm tra trên bàn; giữ các phần trên thành những hàm riêng để sau này tách tệp dễ dàng.

## Dữ liệu chương trình

Mỗi lần đọc cảm biến nên lưu:

- Giá trị đo.
- Cờ hợp lệ.
- Thời điểm đọc gần nhất.

Dữ liệu cần theo dõi:

- Nhiệt độ gió ra từ DS18B20.
- Nhiệt độ và độ ẩm trong khung từ SHT31.
- Nhiệt độ và độ ẩm phòng từ AHT20.
- Trạng thái có khăn sau khi lọc nhiễu TCRT5000.

Nếu dữ liệu cảm biến bắt buộc không hợp lệ hoặc quá cũ, không cho phép gia nhiệt và chuyển sang trạng thái lỗi.

## Trạng thái máy

```text
KIEM_TRA_KHOI_DONG
CHO
KIEM_TRA_GIO
DANG_SAY
LAM_NGUOI
CHO_LAY_KHAN_RA
LOI
```

Trạng thái `LAM_NGUOI` có thể dùng chung cho các trường hợp tháo khăn, sấy xong hoặc hết thời gian chạy tối đa. Sau khi hết thời gian làm nguội an toàn, máy tắt quạt và quay về `CHO`.

Nếu chu kỳ sấy kết thúc nhưng khăn vẫn còn trên giá, dùng `CHO_LAY_KHAN_RA` để tránh tự khởi động lại liên tục. Chỉ quay về `CHO` sau khi xác nhận khăn đã được lấy ra.

## Giả mã vòng lặp điều khiển

```text
khoi_tao:
    tat_tat_ca_quat_va_thanh_nhiet
    khoi_tao_cam_bien_va_cong_giao_tiep
    trang_thai = KIEM_TRA_KHOI_DONG

lap_lien_tuc:
    neu_den_ky_doc_cam_bien:
        doc_tat_ca_cam_bien
        cap_nhat_loc_nhieu_cam_bien_khan

    neu_co_loi_cam_bien_bat_buoc_hoac_du_lieu_qua_cu:
        tat_hai_thanh_nhiet
        chuyen_sang_LOI

    neu_trang_thai == KIEM_TRA_KHOI_DONG:
        neu_cam_bien_hop_le:
            chuyen_sang_CHO

    neu_trang_thai == CHO:
        tat_hai_quat_va_hai_thanh_nhiet
        neu_phat_hien_khan_on_dinh:
            bat_hai_quat
            bat_dau_bo_dem_thoi_gian_chay
            chuyen_sang_KIEM_TRA_GIO

    neu_trang_thai == KIEM_TRA_GIO:
        giu_hai_thanh_nhiet_tat
        neu_gio_on_dinh_va_cam_bien_hop_le_va_nhiet_do_duoi_55:
            chuyen_sang_DANG_SAY
        neu_nhiet_do_tu_55_tro_len:
            chuyen_sang_LOI_QUA_NHIET

    neu_trang_thai == DANG_SAY:
        giu_hai_quat_bat
        neu_khan_bi_thao_on_dinh:
            tat_hai_thanh_nhiet
            bat_dau_lam_nguoi_neu_dieu_kien_an_toan
            chuyen_sang_LAM_NGUOI
        neu_nhiet_do_tu_55_tro_len:
            tat_hai_thanh_nhiet
            chuyen_sang_LOI
        neu_nhiet_do_duoi_40:
            bat_hai_thanh_nhiet
        neu_nhiet_do_tu_45_tro_len:
            tat_hai_thanh_nhiet
        neu_dieu_kien_kho_da_hieu_chinh_duoc_duy_tri_du_lau:
            tat_hai_thanh_nhiet
            bat_dau_lam_nguoi
            chuyen_sang_LAM_NGUOI
        neu_da_het_thoi_gian_chay_toi_da:
            tat_hai_thanh_nhiet
            danh_dau_can_kiem_tra_khan
            bat_dau_lam_nguoi_neu_dieu_kien_an_toan
            chuyen_sang_LAM_NGUOI

    neu_trang_thai == LAM_NGUOI:
        giu_hai_thanh_nhiet_tat
        neu_nhiet_do_hop_le_va_duoi_gioi_han_lam_nguoi:
            giu_quat_bat_cho_den_het_thoi_gian_lam_nguoi
        nguoc_lai:
            tat_hai_quat
        neu_da_lam_nguoi_xong:
            tat_hai_quat
            chuyen_sang_CHO

    neu_trang_thai == LOI:
        tat_hai_thanh_nhiet
        tat_uv
        chi_giu_quat_chay_neu_cam_bien_nhiet_hop_le_va_dieu_kien_an_toan
        chi_reset_sau_khi_nguoi_van_hanh_kiem_tra

    ghi_nhan_du_lieu_neu_den_ky
```

Giả mã thể hiện thứ tự xử lý, chưa phải chương trình có thể nạp vào bo mạch. Khi viết mã thật, ưu tiên dùng bộ định thời theo `millis()` hoặc bộ định thời tương đương thay vì `delay()` dài để vẫn đọc được cảm biến và xử lý lỗi thường xuyên.

## Các quy tắc cần giữ trong mã

- Đặt mọi ngõ ra về mức tắt ngay khi bắt đầu khởi tạo.
- Không bật thanh nhiệt nếu quạt chưa được lệnh chạy hoặc dữ liệu cảm biến cần thiết không hợp lệ. Phần cứng hiện tại chưa xác nhận được quạt có thực sự quay.
- Áp dụng độ trễ nhiệt: bật dưới 40°C, tắt từ 45°C; trong khoảng đó giữ trạng thái trước. Ngưỡng này tạm thời.
- Từ 55°C trở lên, tắt thanh nhiệt và giữ lỗi cho đến khi có người kiểm tra. Đây là ngưỡng phần mềm; không thay thế cầu chì nhiệt độc lập.
- Không coi hết thời gian chạy tối đa là bằng chứng khăn đã khô; báo người vận hành kiểm tra.
- Chưa điều khiển đèn UV trong phiên bản đầu.

## Thông tin cần chốt trước khi viết chương trình nạp được

1. Bo ESP32 chính xác và nền tảng lập trình: Arduino hay ESP-IDF.
2. Chân GPIO dành cho từng cảm biến, quạt và MOSFET thanh nhiệt.
3. Cách nối I2C, địa chỉ SHT31/AHT20 và cách xử lý nếu hai mô-đun trùng địa chỉ.
4. Mức logic bật/tắt của từng mạch MOSFET D4184; kiểm tra bằng đồng hồ trước khi nối tải.
5. Tần suất đọc cảm biến, thời gian lọc nhiễu TCRT5000 và thời gian chờ ổn định luồng gió.
6. Thời gian làm nguội và thời gian chạy tối đa sau các lần thử có giám sát.
7. Cách người vận hành xác nhận/reset lỗi trên mô hình.
