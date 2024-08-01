import sys
import json
import os


def main():
    with open(os.path.expanduser("~/.platformio/platforms/ststm32/platform.json"), "r") as config_file :
        config = json.loads(config_file.read())

    if "stm32" not in config["frameworks"]:
        config["frameworks"]["stm32"] = {
            "package": "framework-ststm32",
            "script": "builder/frameworks/stm32.py"
        }
    
    if "framework-ststm32" not in config["packages"]:
        config["packages"]["framework-ststm32"] = {
            "type": "framework",
            "optional": True,
            "version": "0.0.0"
        }
    
    with open(os.path.expanduser("~/.platformio/platforms/ststm32/platform.json"), "w") as config_file :
        config_file.write(json.dumps(config))

if __name__ == "__main__":
    sys.exit(main())