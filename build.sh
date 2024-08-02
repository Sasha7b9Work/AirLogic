cd ~/airlogic/src2/controller
pio run
cp ./.pio/build/controller/firmware.bin ~/airlogic/out/controller.bin

cd ~/airlogic/src2/mf_lcd
pio run
cp ./.pio/build/MFTech_ALPro_LCD/firmware.bin ~/airlogic/out/lcd.bin

cd ~/airlogic/src2/button
pio run
cp ./.pio/build/ALPRO_button/firmware.bin ~/airlogic/out/button.bin
cp ./.pio/build/ALPRO_button_v2/firmware.bin ~/airlogic/out/button_v2.bin

cd ~/airlogic/src2/boot
pio run
cp ./.pio/build/boot/firmware.bin ~/airlogic/out/boot.bin

cd ~/airlogic/src2/bootloader_f401
pio run
cp ./.pio/build/lcd_boot/firmware.bin ~/airlogic/out/boot_lcd.bin
cp ./.pio/build/lcd_boot_ext/firmware.bin ~/airlogic/out/boot_lcd_ext.bin

cd ~/airlogic/src2/bootloader_f051
pio run
cp ./.pio/build/button_boot_rev1/firmware.bin ~/airlogic/out/boot_button.bin
cp ./.pio/build/button_boot_rev2/firmware.bin ~/airlogic/out/boot_button_v2.bin

cd ~/airlogic/src2/flasher
pio run

cd ~/airlogic
