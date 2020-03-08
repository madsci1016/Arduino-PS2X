## NOTE

fork from： https://github.com/madsci1016/Arduino-PS2X

## PS2X 项目

之前的项目不支持 ESP32，本来打算用，折腾下ESP32，解决相关兼容问题。

调整了下目录结构，https://github.com/MyArduinoLib/Arduino-PS2X-ESP32 直接克隆项目到 /arduino/libraries 目录下。

```
替换：
#ifdef ESP8266
成：
#if defined(ESP8266) || defined(ESP32)
```

## 修改 setup 函数



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

使用ESP32 测试：

https://github.com/MyArduinoLib/Arduino-PS2X-ESP32/blob/master/examples/PS2X_Example_ESP32/PS2X_Example_ESP32.ino

启动日志：

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
```