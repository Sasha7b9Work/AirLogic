cd ~/airlogic/out
rm *.bin

cd ~/airlogic/src/controller
pio run
cp ./.pio/build/controller/firmware.bin ~/airlogic/out/controller.bin

cd ~/airlogic/src/lcd
pio run
cp ./.pio/build/lcd/firmware.bin ~/airlogic/out/lcd.bin

cd ~/airlogic/src/button
pio run
cp ./.pio/build/button/firmware.bin ~/airlogic/out/button.bin
cp ./.pio/build/button_v2/firmware.bin ~/airlogic/out/button_v2.bin

cd ~/airlogic/src/boot
pio run
cp ./.pio/build/boot/firmware.bin ~/airlogic/out/boot.bin

cd ~/airlogic/src/boot_lcd
pio run
cp ./.pio/build/boot_lcd/firmware.bin ~/airlogic/out/boot_lcd.bin
cp ./.pio/build/boot_lcd_ext/firmware.bin ~/airlogic/out/boot_lcd_ext.bin

cd ~/airlogic/src/boot_button
pio run
cp ./.pio/build/boot_button/firmware.bin ~/airlogic/out/boot_button.bin
cp ./.pio/build/boot_button_v2/firmware.bin ~/airlogic/out/boot_button_v2.bin

cd ~/airlogic/src/flasher
pio run

cd ~/airlogic
