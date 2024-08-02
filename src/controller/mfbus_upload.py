Import("env")

env.Replace(
    UPLOADER="./mfupload",
    UPLOADCMD="echo ================================================== && $UPLOADER $UPLOADERFLAGS 'alpro_v10,$BUILD_DIR/${PROGNAME}.bin'"
)
