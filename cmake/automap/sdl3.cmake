# SDL3's build fragment.
# SPDX-License-Identifier: MIT-0
find_package(SDL3 CONFIG REQUIRED)

target_link_libraries(automap_glue PUBLIC SDL3::SDL3)
