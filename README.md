## NOTE

fork from： https://github.com/madsci1016/Arduino-PS2X

## PS2X 项目

之前的项目不支持 ESP32，本来打算用，折腾下ESP32，解决相关兼容问题。

调整了下目录结构，直接克隆项目到 /arduino/libraries 目录下。

```
替换：
#ifdef ESP8266
成：
#if defined(ESP8266) || defined(ESP32)
```