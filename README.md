## Giới thiệu

Thư viện tay cầm PS2 dành cho mạch VIA Makerbot ESP32
Thư viện này giúp các bạn có thể sử dụng tay cầm PS2 cho mạch VIA Makerbot

Thư viện này là 1 bản sao thư viện tay cầm PS2 của 2 tác giả [madsci1016](https://github.com/madsci1016/Arduino-PS2X) và [MyArduinoLib](https://github.com/MyArduinoLib/Arduino-PS2X-ESP32) được chỉnh sửa lại để sử dụng với mạch VIA Makerbot ESP32

## Sử dụng thư viện PSX2 với mạch Makerbot BANHMI



## Khởi tạo thư viện:



```
  while (error != 0) {
    delay(1000);// 1 second wait
    //setup pins and settings: GamePad(clock, command, attention, data, Pressures?, Rumble?) check for error
    error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, pressures, rumble);
    Serial.print("#try config ");
    Serial.println(tryNum);
    tryNum ++;
  }
```

<!-- Ví dụ chạy với mạch VIA Makerbot BANHMI：

https://github.com/MyArduinoLib/Arduino-PS2X-ESP32/blob/master/examples/PS2X_Example_ESP32/PS2X_Example_ESP32.ino

Thông tin hiện ra trên Serial monitor Khi chạy ví dụ trên：

```
15:41:44.363 -> #try config 1
15:41:45.392 -> #try config 2
15:41:45.392 -> 73
15:41:45.392 -> Controller_type: 3
15:41:45.392 ->  DualShock Controller found 
15:41:45.392 -> Start is being held
15:41:45.392 -> Select is being held
15:41:45.392 -> Up held this hard: 0
15:41:45.392 -> Right held this hard: 0
15:41:45.392 -> LEFT held this hard: 0
15:41:45.392 -> DOWN held this hard: 0
15:41:45.392 -> Stick Values:0,0,0,0
15:41:45.459 -> × just changed
15:41:45.459 -> □ just released
``` -->

