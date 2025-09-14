cmake_minimum_required(VERSION 3.20)
project(TalksterUnwindowed)

set(CMAKE_CXX_STANDARD 26)

add_executable(TalksterUnwindowed main.cpp
        window/ChatWindow.cpp
        window/ChatWindow.h
        renderer/Renderer.h
        TextBuffer.h
        HotkeyManager.h
        renderer/Renderer.cpp
        TextBuffer.cpp
        window/MessageWindow.cpp
        window/MessageWindow.h
        renderer/MessageRenderer.cpp
        renderer/MessageRenderer.h
        client/WebSocketClient.cpp
        client/WebSocketClient.h
        client/MatrixClient.cpp
        client/MatrixClient.h)

target_include_directories(TalksterUnwindowed PRIVATE ${CMAKE_SOURCE_DIR}/external)

# Add this line to specify WinMain as the entry point
set_target_properties(TalksterUnwindowed PROPERTIES WIN32_EXECUTABLE TRUE)

# Link against Direct2D and other Windows libraries
target_link_libraries(TalksterUnwindowed PRIVATE d2d1 dwrite.lib ws2_32)
