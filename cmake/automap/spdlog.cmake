# spdlog's build fragment.
# SPDX-License-Identifier: MIT-0
find_package(spdlog CONFIG REQUIRED)

target_link_libraries(automap_glue PUBLIC spdlog::spdlog)
