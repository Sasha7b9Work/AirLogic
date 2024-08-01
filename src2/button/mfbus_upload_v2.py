Import("env")

env.Replace(
    UPLOADER="./mfupload",
    UPLOADCMD="echo ================================================== && $UPLOADER $UPLOADERFLAGS 'alpro_v10_button_v2,$BUILD_DIR/${PROGNAME}.bin'"
)
