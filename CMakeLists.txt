cmake_minimum_required(VERSION 3.20)
project(TalksterUnwindowed)

set(CMAKE_CXX_STANDARD 26)

add_executable(TalksterUnwindowed main.cpp)

# Add this line to specify WinMain as the entry point
set_target_properties(TalksterUnwindowed PROPERTIES WIN32_EXECUTABLE TRUE)

# Link against Direct2D and other Windows libraries
target_link_libraries(TalksterUnwindowed PRIVATE d2d1)