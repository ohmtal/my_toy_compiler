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
bison_target(MyParser toy/parser.y
    ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.hpp
)

flex_target(MyLexer toy/tokens.l
    ${CMAKE_CURRENT_BINARY_DIR}/tokens.cpp
)

add_flex_bison_dependency(MyLexer MyParser)

# --------------------------------------------------------
#  Executable
# --------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")

set(TARGET_NAME parser)

add_executable(${TARGET_NAME}
    toy/main.cpp
    toy/codegen.cpp
    toy/runtime.cpp
    toy/native.cpp
    ${BISON_MyParser_OUTPUTS}
    ${FLEX_MyLexer_OUTPUTS}
)



target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/toy
)

# --------------------------------------------------------
# Link LLVM and System Libraries
# --------------------------------------------------------

target_link_libraries(${TARGET_NAME} PRIVATE
    LLVM
    Threads::Threads
    ${CMAKE_DL_LIBS}
)
