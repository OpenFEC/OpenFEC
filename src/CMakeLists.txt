file (GLOB_RECURSE openfec_sources *)

add_library(openfec  SHARED  ${openfec_sources})

# Feel free to edit as appropriate the "target_link_libraries".
#
# Add the IL (DevIL) library if you want to have the possibility to
# produce H/G matrix images with LDPC codes.
# More precisely, we are relying on the DevIL library
# (http://openil.sourceforge.net/).
# Please install it on your machine before compiling the OpenFEC
# library if needed.
# Otherwise remove the IL library.

target_link_libraries(openfec m)
#target_link_libraries(openfec m IL)

#target_link_libraries(openfec pthread IL)
#target_link_libraries(openfec pthread)
