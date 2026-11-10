# stb_truetype as an in-repo library for Automap's private map.
#
# The header itself comes from the vcpkg stb port, which the public map's
# stb-image already brings into the composition. Naming that port a second
# time, from a second map library, makes Automap 0.1.0 write it twice into
# vcpkg.json, which vcpkg rejects; so this directory only defines the target.
find_package(Stb REQUIRED)
add_library(stb_truetype INTERFACE)
add_library(stb::truetype ALIAS stb_truetype)
target_include_directories(stb_truetype INTERFACE "${Stb_INCLUDE_DIR}")
