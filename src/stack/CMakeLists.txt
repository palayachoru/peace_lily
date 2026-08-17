message("-- Processing: ${CMAKE_CURRENT_LIST_FILE}")

target_sources(plily PRIVATE stack.c)
target_sources(plily PRIVATE stackll.c)
