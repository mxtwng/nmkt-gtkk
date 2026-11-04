# Firmware ESP32

Chu muc nay la mo phong phan mem cho luu do dieu khien. Chuong trinh nhan lenh qua Serial de gia lap cam bien va in trang thai dau ra ao. No khong gan GPIO, khong doc cam bien that va khong the dieu khien quat hay thanh PTC.

## Board

`platformio.ini` co hai moi truong build:

- `esp32_devkitc_v4` dung cho ESP32 DevKitC V4 voi module WROOM-32D.
- `nodemcu_32s` dung cho AI-Thinker NodeMCU-32S V1.3.

PlatformIO mac dinh build cho ca hai board.

## Chay mo phong

1. Mo thu muc goc cua repo (`nmkt`) trong VS Code.
2. Chon board trong PlatformIO va dung **Build**, sau do **Upload**.
3. Mo Serial Monitor o toc do 115200 baud.
4. Nhap `help` de xem lenh. Cac lenh mo phong gom:

```text
sensors ok|fail
towel on|off
 temp <do_C>
 dry on|off
 status
 reset
```

Bo dem trong mo phong duoc rut ngan de de thu bang tay: cho gio 3 giay, xac nhan dieu kien kho 15 giay, lam nguoi 5 giay, gioi han chu ky 120 giay. Day khong phai cac gia tri de dua vao may that.

## Cach thu thu cong

- Nhap `towel on`; sau loc nhieu, may vao kiem tra gio, roi chuyen sang dang say.
- Nhap `temp 42`, `temp 45`, `temp 55` de quan sat do tre dieu khien va loi qua nhiet.
- Nhap `dry on` va de duy tri qua 15 giay de mo phong xac nhan kho.
- Nhap `sensors fail` de mo phong loi cam bien; dung `sensors ok` roi `reset` de xoa loi.
- Sau khi ket thuc chu ky, neu van con khan, may doi den khi nhap `towel off` truoc khi cho chu ky moi.

## Buoc tiep theo

Khi cac cam bien va mach cong suat san sang, bo phan doc lenh mo phong se duoc thay bang driver cam bien. Sau do moi gan GPIO cho dau ra ao, theo bang chan da duoc doi chieu voi board va mach MOSFET thuc te. Khong dung phien ban mo phong de van hanh may say that.
