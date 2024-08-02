cd ~/airlogic/src2/mftech-f405
pio run

cd ~/airlogic/src2/mf_lcd
pio run

cd ~/airlogic/src2/button
pio run

cd ~/airlogic/src2/bootloader
pio run
cp ./.pio/build/mfBoot/firmware.bin ~/airlogic/out/boot.bin

cd ~/airlogic/src2/bootloader_f401
pio run

cd ~/airlogic/src2/bootloader_f051
pio run
cp ./.pio/build/button_boot_rev1/firmware.bin ~/airlogic/out/boot_button_v1.bin
cp ./.pio/build/button_boot_rev2/firmware.bin ~/airlogic/out/boot_button_v2.bin

cd ~/airlogic/src2/flasher
pio run

cd ~/airlogic
