cmake_minimum_required(VERSION 3.20)
project(z_buffer)

set(CMAKE_CXX_STANDARD 14)

# 设置OpenCV目录所在
set(OpenCV_DIR "D:\\opencv4\\install")

# 搜索OpenCV目录
find_package(OpenCV REQUIRED)

# 添加OpenCV头文件目录
# not necessary for newly cmake
#include_directories(${OpenCV_INCLUDE_DIRS})


add_executable(z_buffer main.cpp ZBuffer.cpp Model.cpp)
target_link_libraries(z_buffer ${OpenCV_LIBS})
