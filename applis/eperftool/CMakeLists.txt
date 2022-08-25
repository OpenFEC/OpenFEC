file (GLOB eperftool_sources ./*)

set(EPERFTOOL_BIN ${PROJECT_BINARY_DIR}/applis/eperftool/eperftool CACHE STRING "eperftool dir")
add_executable( eperftool ${eperftool_sources})


target_link_libraries( eperftool openfec m)

install(TARGETS eperftool)
