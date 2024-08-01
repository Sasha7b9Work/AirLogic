wget -O get-platformio.py https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py
python3 get-platformio.py

sudo mkdir -p /usr/local/bin
sudo ln -s ~/.platformio/penv/bin/platformio /usr/local/bin/platformio
sudo ln -s ~/.platformio/penv/bin/pio /usr/local/bin/pio
sudo ln -s ~/.platformio/penv/bin/piodebuggdb /usr/local/bin/piodebuggdb

# Устанавливаем компоненты [Platformio](https://platformio.org/)
pio pkg install -g --platform "platformio/ststm32@^15.4.1"

# Копируем собственную реализацию [Arduino_Core_STM32](https://github.com/stm32duino/Arduino_Core_STM32) (коммит fe500808f773c00f537ba3242ed18e0bdc5008f9)
cp -r ~/airlogic/src/framework-ststm32 ~/.platformio/packages/

cp ~/airlogic/new_files/platforms/ststm32/platform.json ~/.platformio/platforms/ststm32
cp ~/airlogic/new_files/platforms/ststm32/builder/frameworks/stm32.py ~/.platformio/platforms/ststm32/builder/frameworks
