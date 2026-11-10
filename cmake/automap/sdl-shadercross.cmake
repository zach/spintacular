# SDL_shadercross's build fragment.
# SPDX-License-Identifier: MIT-0
# SDL_shadercross's static library calls SDL3 and its package finds neither
# SDL3's headers nor its library itself.
find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_shadercross CONFIG REQUIRED)

target_link_libraries(automap_glue PUBLIC SDL3_shadercross::SDL3_shadercross SDL3::SDL3)
