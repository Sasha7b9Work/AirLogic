import sys
import json
import os


def main():
    with open(os.path.expanduser("~/.platformio/packages/framework-ststm32/package.json"), "w") as config_file :
        config_file.write(json.dumps({
            "name": "framework-ststm32",
            "description": "Wiring-based Framework (STM32 Core)",
            "version": "0.0.0"
        }))

if __name__ == "__main__":
    sys.exit(main())