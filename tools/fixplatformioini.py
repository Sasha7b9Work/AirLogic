import sys
import configparser
import sys


def main():
    file_path = sys.argv[1]
    section_name = sys.argv[2]

    config = configparser.ConfigParser()
    config.read(file_path)

    config[section_name]["platform_packages"] = """
        toolchain-gccarmnoneeabi@1.90201.191206
        framework-ststm32@0.0.0
    """

    with open(file_path, 'w') as configfile:
        config.write(configfile)

if __name__ == "__main__":
    sys.exit(main())