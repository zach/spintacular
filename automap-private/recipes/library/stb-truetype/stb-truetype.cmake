# stb_truetype's build fragment. The stb port installs the stb headers and a
# FindStb module but no imported target, so the fragment defines stb::truetype.
find_package(Stb REQUIRED)
if(NOT TARGET stb::truetype)
    add_library(stb::truetype INTERFACE IMPORTED)
    set_target_properties(stb::truetype PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${Stb_INCLUDE_DIR}")
endif()

target_link_libraries(automap_glue PUBLIC stb::truetype)
