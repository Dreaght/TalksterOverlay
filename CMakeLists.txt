cmake_minimum_required(VERSION 3.20)
project(TalksterUnwindowed)

set(CMAKE_CXX_STANDARD 26)

# -------------------- Source Files --------------------
add_executable(TalksterUnwindowed
        main.cpp
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
        client/MatrixClient.h
        Utils.h)

# -------------------- Include Directories --------------------
target_include_directories(TalksterUnwindowed PRIVATE
        ${CMAKE_SOURCE_DIR}/external
        ${CMAKE_SOURCE_DIR}/external/nlohmann
)

# -------------------- Windows Subsystem --------------------
# Ensures WinMain is used instead of main
set_target_properties(TalksterUnwindowed PROPERTIES WIN32_EXECUTABLE TRUE)

# -------------------- Link Windows Libraries --------------------
target_link_libraries(TalksterUnwindowed PRIVATE d2d1 dwrite.lib ws2_32)

# -------------------- Release Build Optimizations --------------------
if(MSVC)
    # Compiler flags
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /O1 /Os /DNDEBUG /GL")

    # Linker flags
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /INCREMENTAL:NO /LTCG /OPT:REF /OPT:ICF")
endif()
