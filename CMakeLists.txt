cmake_minimum_required(VERSION 3.15)
project(my_toy_compiler LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --------------------------------------------------------
# Dependencies (LLVM, Flex, Bison)
# --------------------------------------------------------
find_package(LLVM REQUIRED CONFIG)
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# --------------------------------------------------------
#  Flex and Bison Files
# --------------------------------------------------------
bison_target(MyParser parser.y
    ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.hpp
)

flex_target(MyLexer tokens.l
    ${CMAKE_CURRENT_BINARY_DIR}/tokens.cpp
)

add_flex_bison_dependency(MyLexer MyParser)

# --------------------------------------------------------
#  Executable
# --------------------------------------------------------
add_executable(parser
    main.cpp
    codegen.cpp
    corefn.cpp
    native.cpp
    ${BISON_MyParser_OUTPUTS}
    ${FLEX_MyLexer_OUTPUTS}
)

target_include_directories(parser PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# --------------------------------------------------------
# Link LLVM and System Libraries
# --------------------------------------------------------
# llvm_map_components_to_libnames(llvm_libs core support native codegen)

# target_link_libraries(parser PRIVATE
#     ${llvm_libs}
#     Threads::Threads
#     ${CMAKE_DL_LIBS}
# )

# llvm_config(parser core support native codegen)
#
# target_link_libraries(parser PRIVATE
#     ${LLVM_AVAILABLE_LIBS}
#     Threads::Threads
#     ${CMAKE_DL_LIBS}
# )

target_link_libraries(parser PRIVATE
    LLVM
    Threads::Threads
    ${CMAKE_DL_LIBS}
)
