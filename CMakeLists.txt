cmake_minimum_required(VERSION 3.16)
project(CountdownOvertimer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)

# Add the executable
add_executable(CountdownOvertimer main.cpp)

# Link Qt libraries

target_link_libraries(CountdownOvertimer PRIVATE Qt6::Widgets Qt6::Multimedia)

