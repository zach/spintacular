# GLM's build fragment.
# SPDX-License-Identifier: MIT-0
find_package(glm CONFIG REQUIRED)

target_link_libraries(automap_glue PUBLIC glm::glm)
