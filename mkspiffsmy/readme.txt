Запис файлів в образ
.\mkspiffs.exe -c files -b 4096 -p 256 -s 0x1E0000 spiffs1.bin
або
mkspiffs -c files -b 4096 -p 256 -s 0x1E0000 spiffs1.bin

Залити образ в память
esptool --chip esp32 --port COM5 --baud 921600 write_flash -z 0x110000 spiffs1.bin

Прочитати образ з памяті
esptool -b 921600 --port COM5 read_flash 0x110000 0x1E0000 spiffs1.bin

Пhочитати файли з образа
.\mkspiffs.exe -u files spiffs1.bin

зтерти флеш 
esptool erase_flash