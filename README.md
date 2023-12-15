# --- ESP32 4Mb + GPS module(GY-GPS6MV2) ---
Data logging using  built internal flash memory (SPIFFs).


Example log asile into file:

2023:12:15 12:55:21 latitude:49.5XXXXN longitude:23.9XXXXE altitude:264.30m speed:0.07m

2023:12:15 12:55:22 latitude:49.5XXXXN longitude:23.9XXXXE altitude:264.30m speed:0.07m

...

--------------------------------------------------------------------------
For work with SPIFFs using esptool.exe and mkspiffs.exe. They locate into mkspiffsmy folder.

Write files from "files" folder into spiffs1.bin image file.

.\mkspiffs.exe -c files -b 4096 -p 256 -s 0x1E0000 spiffs1.bin

or

mkspiffs -c files -b 4096 -p 256 -s 0x1E0000 spiffs1.bin

Write file image spiffs1.bin into flash memory.

esptool --chip esp32 --port COM5 --baud 921600 write_flash -z 0x110000 spiffs1.bin


Read data from ESP32 flash memory and make image file spiffs1.bin.

RUN: esptool -b 921600 --port COM5 read_flash 0x110000 0x1E0000 spiffs1.bin

Red file from image spiffs1.bin and save it into "files" folder.

RUN: .\mkspiffs.exe -u files spiffs1.bin

Ease all flash.

RUN: esptool erase_flash

 
