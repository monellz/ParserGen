aux_source_directory(. TEST_SRCS)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${GTEST_INCLUDE_DIR})

foreach(SRC ${TEST_SRCS})
  string(REGEX REPLACE "\./*(.*)\.cpp$" "\\1\.test" OUT ${SRC})
  add_executable(${OUT} ${SRC}
    ${PROJECT_SOURCE_DIR}/src/core/nfa.cpp
    ${PROJECT_SOURCE_DIR}/src/core/dfa.cpp
    ${PROJECT_SOURCE_DIR}/src/core/re.cpp
  )
  target_link_libraries(${OUT} ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY})
  add_test(NAME ${OUT} COMMAND ${OUT})
endforeach()
