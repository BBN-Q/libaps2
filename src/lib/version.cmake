# create or update the version header file with the latest git describe
execute_process(
    COMMAND git describe --dirty
    OUTPUT_VARIABLE GIT_DESCRIBE
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
configure_file(${SRC} ${DST} @ONLY)
