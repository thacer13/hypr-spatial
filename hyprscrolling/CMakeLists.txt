cmake_minimum_required(VERSION 3.27)

project(hyprscrolling
    DESCRIPTION "hyprscrolling plugin for Hyprland"
    VERSION 0.1
)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE SRC "*.cpp")

add_library(hyprscrolling SHARED ${SRC})

find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET
    hyprland
    hyprutils    # <-- Added this because the original code uses it
    libdrm
    libinput
    libudev
    pangocairo
    pixman-1
    wayland-server
    xkbcommon
)

# --- THE FIX (Bulletproof Version) ---
# Ask pkg-config exactly where hyprland's headers are
execute_process(
    COMMAND pkg-config --variable=includedir hyprland
    OUTPUT_VARIABLE HYPRLAND_INCLUDE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# If it still comes up empty, fallback to the standard Linux path
if("${HYPRLAND_INCLUDE_DIR}" STREQUAL "")
    set(HYPRLAND_INCLUDE_DIR "/usr/include/hyprland")
endif()

# Get the parent directory (e.g., /usr/include) safely wrapped in quotes
get_filename_component(HYPRLAND_PARENT_DIR "${HYPRLAND_INCLUDE_DIR}" DIRECTORY)

# Tell our target about it
target_include_directories(hyprscrolling PRIVATE "${HYPRLAND_PARENT_DIR}")
# ---------------
# 
target_link_libraries(hyprscrolling PRIVATE rt PkgConfig::deps)

install(TARGETS hyprscrolling)
