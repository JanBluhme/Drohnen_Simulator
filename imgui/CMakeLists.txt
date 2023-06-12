cmake_minimum_required(VERSION 3.11)

add_library(imgui
    src/imgui/imgui.cpp
    src/imgui/imgui_draw.cpp
    src/imgui/imgui_tables.cpp
    src/imgui/imgui_widgets.cpp
    src/imgui/imgui_impl_sdl.cpp
    src/imgui/imgui_impl_opengl2.cpp)

set_property(TARGET imgui PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(imgui PUBLIC src)
target_add_default_build_options(imgui PUBLIC)
target_link_libraries(imgui PUBLIC SDL2::SDL2)

