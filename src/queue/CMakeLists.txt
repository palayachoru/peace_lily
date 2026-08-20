message("-- Processing: ${CMAKE_CURRENT_LIST_FILE}")

target_sources(plily PRIVATE queue.c)
target_sources(plily PRIVATE queuell.c)
