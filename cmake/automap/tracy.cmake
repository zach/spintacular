# Tracy's build fragment. Tracy::TracyClient's interface carries the options
# the client was built with, TRACY_ENABLE among them.
# SPDX-License-Identifier: MIT-0
find_package(Tracy CONFIG REQUIRED)

target_link_libraries(automap_glue PUBLIC Tracy::TracyClient)
