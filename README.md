# Máy tự động sấy và khử khuẩn khăn

Dự án thiết kế một giá treo khăn có quạt và bộ gia nhiệt để thổi khí ấm qua khăn trong phòng tắm. ESP32 dự kiến đọc cảm biến nhiệt độ, độ ẩm và phát hiện khăn, sau đó điều khiển quạt và bộ gia nhiệt để sấy khăn rồi tự dừng. Mục tiêu là giảm thời gian khăn ẩm khi không có chỗ phơi ngoài trời.

Hiện repo đang ở giai đoạn lập kế hoạch phần cứng và quy trình thử nghiệm. Chưa có firmware hay mã điều khiển trong repo; các thông số về thời gian sấy và hiệu quả cần được đo trên mô hình thực tế.

## Nguyên lý dự kiến

1. TCRT5000 phát hiện khăn đang được treo.
2. ESP32 bật quạt và điều khiển các thanh PTC để duy trì luồng khí ấm khoảng 40–45°C.
3. DS18B20 theo dõi nhiệt độ khí; SHT31 đo độ ẩm trong khung và AHT20 đo độ ẩm phòng.
4. Khi khăn khô theo tiêu chí độ ẩm đã chọn, hệ thống tắt bộ gia nhiệt và quạt.

Thời gian sấy 2–4 giờ cho khăn 70 × 140 cm hiện chỉ là ước tính, cần được kiểm chứng bằng thử nghiệm và ghi số liệu.

## Phần cứng dự kiến

| Bộ phận | Vai trò |
|---|---|
| ESP32 | Điều khiển và đọc cảm biến |
| Hai quạt 12 V | Tạo luồng khí qua các ống đáy |
| Hai thanh gia nhiệt PTC 12 V | Làm ấm luồng khí |
| DS18B20 | Đo nhiệt độ khí ra |
| SHT31 và AHT20 | So sánh độ ẩm trong khung và độ ẩm phòng |
| TCRT5000 | Phát hiện khăn |
| Mạch MOSFET D4184 | Đóng/ngắt tải DC theo thiết kế |
| Nguồn 12 V và cầu chì | Cấp nguồn và bảo vệ quá dòng |
| Khung ống | Đỡ khăn và phân phối khí ấm |

Thông số và số lượng linh kiện vẫn cần được xác nhận trước khi mua. Đặc biệt, phải xác định công suất/dòng của PTC và chọn nguồn, dây dẫn, cầu chì phù hợp.

## Bố trí mô hình

Khung dự kiến dạng tam giác, dài khoảng 80 cm. Khăn gấp qua ống đỉnh và cách các lỗ thổi ở ống đáy khoảng 3 cm để luồng khí không bị che. Các chi tiết kích thước vật liệu cần được kiểm tra lại trên linh kiện thực tế trước khi cắt hoặc lắp.

## Cấu trúc repo

```text
├── KhochtrinkhaidnMytngsyvkhkhunkhn.docx  kế hoạch và ghi chú thiết kế ban đầu
├── README.md                              tổng quan dự án
└── .gitignore                             tệp và thư mục cục bộ không đưa lên Git
```

Firmware, sơ đồ đấu nối, bản vẽ khung, bảng thử nghiệm và báo cáo sẽ được bổ sung khi hoàn thiện.

## An toàn

- Chỉ người phụ trách kỹ thuật được đấu dây hoặc cấp điện cho mô hình.
- Đặt bộ nguồn AC–DC ngoài khu vực ẩm; chỉ đưa điện áp thấp DC vào khung.
- Dùng cầu chì nguồn và cầu chì nhiệt độc lập đặt sau mỗi PTC.
- Quạt phải chạy khi PTC đang bật. Tắt PTC nếu quá nhiệt hoặc cảm biến nhiệt bị lỗi.
- Không để PTC chạm vào ống nhựa; tránh PVC gần bộ gia nhiệt.
- Chỉ lắp UV-C nếu xác nhận đúng bước sóng và có thiết kế che chắn liên động. UV-C gây hại cho mắt và da; không nhìn trực tiếp và chỉ thử trong hộp kín.
- Không vận hành thiết bị trong phòng tắm khi chưa được người phụ trách kiểm tra an toàn.

Các giá trị nhiệt độ và cấu hình trong tài liệu là mục tiêu thiết kế ban đầu, không phải xác nhận an toàn của sản phẩm hoàn chỉnh.

## Việc cần chốt

- Xác nhận công suất và dòng điện của từng thanh PTC, khả năng nguồn, cầu chì và dây dẫn.
- Kiểm tra kích thước trong của ống và lưu lượng gió thực tế.
- Xác nhận loại tiếp điểm của cầu chì nhiệt.
- Đo nhiệt độ, độ ẩm và thời gian sấy với khăn ướt; xây dựng tiêu chí dừng sấy.
- Quyết định có bỏ UV-C hay không. Phần sấy vẫn hoạt động độc lập nếu không dùng UV-C.

## Giấy phép

Chưa kèm giấy phép. Nhóm nên thống nhất điều kiện sử dụng và chia sẻ mã nguồn trước khi chọn giấy phép.
