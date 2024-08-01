Import("env")

env.Replace(
    UPLOADER="./mfupload",
    UPLOADCMD="echo ================================================== && $UPLOADER $UPLOADERFLAGS 'alpro_v10_lcd,$BUILD_DIR/${PROGNAME}.bin'"
)
