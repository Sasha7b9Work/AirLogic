## Сборка проекта mftech-f405

11. Произвести сборку проекта mftech-f405
    ``` bash
    cd  ~/airlogic/src2/mftech-f405
    pio run
    ```

    ![mftech-f405-build-sucess](imgs/mftech-f405_build.jpg)

## Сборка mf_lcd

Все предыдущие шаги и:

1. Добавляем конфигурацию для плат в проект
    ``` bash 
    mkdir ~/airlogic/src2/mf_lcd/boards 
    cp ~/airlogic/src/mftech_alpro10_lcd.json ~/airlogic/src2/mf_lcd/boards
    ``` 
2. Отредактировать файл `~/airlogic/src2/mf_lcd/platformio.ini` конфигурации сборки проекта mf_lcd следующим образом:
    ``` ini
    platform_packages =
        toolchain-gccarmnoneeabi@1.90201.191206
        framework-ststm32@0.0.0
    ```

3. Исправление ошибок проекта:
    * Заменить `~/.platformio/packages/framework-ststm32/variants/MFTECH_LCD/PeripheralPins.c` заменить Arduino.h на sketch.h
    * Заменить `~/.platformio/packages/framework-ststm32/variants/MFTECH_LCD/variant.cpp` заменить pins_arduino.h на pins_stm.h
    * Скопировать недостающую библиотеку `cp -r ~/airlogic/src2/flasher/lib/CircularBuffer ~/airlogic/src2/mf_lcd/lib/`

## Сборка button

Все предыдущие шаги и:

1. Добавляем конфигурацию для плат в проект
    ``` bash 
    mkdir ~/airlogic/src2/button/boards 
    cp ~/airlogic/src/test_f051.json ~/airlogic/src2/button/boards
    ``` 
2. Отредактировать файл `~/airlogic/src2/button/platformio.ini` конфигурации сборки проекта button следующим образом:
    ``` ini
    platform_packages =
        toolchain-gccarmnoneeabi@1.90201.191206
        framework-ststm32@0.0.0
    ```

3. Исправление ошибок проекта:
    * Заменить `~/.platformio/packages/framework-ststm32/variants/MFTECH_BUTTON/PeripheralPins.c` заменить Arduino.h на sketch.h
    * Заменить `~/.platformio/packages/framework-ststm32/variants/MFTECH_BUTTON/variant.cpp` заменить pins_arduino.h на pins_stm.h

## Сборка bootloader

Все предыдущие шаги и:

1. Добавляем конфигурацию для плат в проект
    ``` bash 
    mkdir ~/airlogic/src2/bootloader/boards 
    cp ~/airlogic/src/mftech_alpro10.json ~/airlogic/src2/bootloader/boards
    ``` 
2. Отредактировать файл `~/airlogic/src2/bootloader/platformio.ini` конфигурации сборки проекта bootloader следующим образом:
    ``` ini
    platform_packages =
        toolchain-gccarmnoneeabi@1.90201.191206
        framework-ststm32@0.0.0
    ```

3. Исправление ошибок проекта:
    * Скопировать недостающую библиотеку `cp -r ~/airlogic/src2/flasher/lib/CircularBuffer ~/airlogic/src2/bootloader/lib/`
    * Скопировать недостающую библиотеку `cp -r ~/airlogic/src/mfproto/lib/Crypto ~/airlogic/src2/bootloader/lib/`

## Сборка bootloader_f051

Все предыдущие шаги и:

1. Добавляем конфигурацию для плат в проект
    ``` bash 
    mkdir ~/airlogic/src2/bootloader_f051/boards 
    cp ~/airlogic/src/test_f051.json ~/airlogic/src2/bootloader_f051/boards
    ``` 
2. Отредактировать файл `~/airlogic/src2/bootloader_f051/platformio.ini` конфигурации сборки проекта bootloader_f051 следующим образом:
    ``` ini
    platform_packages =
        toolchain-gccarmnoneeabi@1.90201.191206
        framework-ststm32@0.0.0
    ```

3. Исправление ошибок проекта:
    * Скопировать недостающую библиотеку `cp -r ~/airlogic/src2/flasher/lib/CircularBuffer ~/airlogic/src2/bootloader_f051/lib/`
    * Скопировать недостающую библиотеку `cp -r ~/airlogic/src/mfproto/lib/Crypto ~/airlogic/src2/bootloader_f051/lib/`

## Сборка flasher

Проект собирается только под Linux!

Все предыдущие шаги и:

1. Исправление ошибок проекта:
    * Скопировать недостающую библиотеку `cp -r ~/airlogic/src/mfproto/lib/Crypto ~/airlogic/src2/flasher/lib/`