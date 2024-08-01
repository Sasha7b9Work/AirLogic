Import("env")

env.Replace(
    UPLOADER="./mfupload",
    UPLOADCMD="echo ================================================== && $UPLOADER $UPLOADERFLAGS 'alpro_v10_button,$BUILD_DIR/${PROGNAME}.bin'"
)
