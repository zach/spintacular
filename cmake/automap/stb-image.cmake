# stb_image's build fragment. The stb port installs the stb headers and a
# FindStb module but no imported target, so the fragment defines stb::image.
# SPDX-License-Identifier: MIT-0
find_package(Stb REQUIRED)
if(NOT TARGET stb::image)
    add_library(stb::image INTERFACE IMPORTED)
    set_target_properties(stb::image PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${Stb_INCLUDE_DIR}")
endif()

target_link_libraries(automap_glue PUBLIC stb::image)
