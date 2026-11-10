# stb_truetype's build fragment: links the in-repo stb::truetype target
# (third_party/stb-truetype) into the glue target.
target_link_libraries(automap_glue PUBLIC stb::truetype)
