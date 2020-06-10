# ESP32 Connected TFT Display

Connect an ESP32 with an LCD via MQTT and draw 40x25 dots. The picture shows an example for a Mandelbrot set:

![Mandelbrot Set on an ESP-WROVER-KIT](https://pbs.twimg.com/media/EZ1wpMRXQAY0kwy?format=jpg)

([tweet](https://twitter.com/choas/status/1269294570057187329))

## TTF Library

TTF library based on [ESP32_TFT_library](https://github.com/AMTechMX/ESP32_TFT_library.git)([66e39dd](https://github.com/AMTechMX/ESP32_TFT_library/commit/66e39dd13e61891423638b50686df200877b8270))

## Build

requires esp-idf

### Menuconfig

Configure your LCD display, MQTT broker and topic.

```shell
idf.py menuconfig
```

### Build, Flash and Monitor

```shell
idf.py flash monitor
```
