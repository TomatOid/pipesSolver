project(pipesSolver)

set(SOURCE_FILES main.c)

add_executable(solve ${SOURCE_FILES})
target_link_libraries(solve m curl c)
