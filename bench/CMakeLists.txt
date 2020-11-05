aux_source_directory(. BENCH_SRCS)

include_directories(${PROJECT_SOURCE_DIR}/src/core)

foreach(SRC ${BENCH_SRCS})
  string(REGEX REPLACE "\./*(.*)\.cpp$" "\\1\.bench" OUT ${SRC})
  add_executable(${OUT} ${SRC})
  target_link_libraries(${OUT} benchmark::benchmark)
  add_test(NAME ${OUT} COMMAND ${OUT})
endforeach()
