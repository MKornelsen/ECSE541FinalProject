cmake_minimum_required(VERSION 3.0.0)
project(Assignment2 VERSION 0.1.0)

include(CTest)
enable_testing()

include_directories("C:/Users/murra/Documents/SystemC/systemc-2.3.3/src")
link_directories("C:/Users/murra/Documents/SystemC/systemc-2.3.3/msvc10/SystemC/x64/Release/")
add_compile_options("/vmg" "/GR")

add_executable(MatMul matmul.cpp)
add_executable(MatMulClocked matmul_clocked.cpp)
add_executable(MatMulNoHW matmul_no_hw.cpp)
add_executable(MatMulFullHW matmul_fullmat_hw.cpp)

target_link_libraries(MatMul systemc)
target_link_libraries(MatMulClocked systemc)
target_link_libraries(MatMulNoHW systemc)
target_link_libraries(MatMulFullHW systemc)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
