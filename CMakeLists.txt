cmake_minimum_required(VERSION 3.0.0)
project(541FinalProject VERSION 0.1.0)

include(CTest)
enable_testing()

include_directories(include SystemC/include)
add_compile_options("/vmg" "/GR")

add_library(sysc STATIC IMPORTED)
set_target_properties(sysc PROPERTIES IMPORTED_LOCATION_DEBUG "${CMAKE_SOURCE_DIR}/SystemC/Debug/SystemC.lib")
set_target_properties(sysc PROPERTIES IMPORTED_LOCATION_RELEASE "${CMAKE_SOURCE_DIR}/SystemC/Release/SystemC.lib")

add_executable(Testbench src/main.cpp)
add_executable(AccTB src/accelerator_tb.cpp)
add_executable(EIEMain src/eie_main.cpp)

target_link_libraries(Testbench sysc)
target_link_libraries(AccTB sysc)
target_link_libraries(EIEMain sysc)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
