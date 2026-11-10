# miniaudio's build fragment. The miniaudio port installs miniaudio.h and no
# CMake package, so the fragment defines the imported target glue links to.
# miniaudio loads each platform's audio libraries at run time; it needs
# threads, and on Linux libdl and libm.
# SPDX-License-Identifier: MIT-0
find_path(MINIAUDIO_INCLUDE_DIR miniaudio.h REQUIRED)
find_package(Threads REQUIRED)
if(NOT TARGET miniaudio::miniaudio)
    add_library(miniaudio::miniaudio INTERFACE IMPORTED)
    set_target_properties(miniaudio::miniaudio PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${MINIAUDIO_INCLUDE_DIR}")
    target_link_libraries(miniaudio::miniaudio INTERFACE Threads::Threads ${CMAKE_DL_LIBS} $<$<PLATFORM_ID:Linux>:m>)
endif()

target_link_libraries(automap_glue PUBLIC miniaudio::miniaudio)
