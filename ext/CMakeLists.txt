# This file configures how external libraries in the ext/ directory are built or found.

# --- GLFW ---
if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/glfw/CMakeLists.txt")
    message(FATAL_ERROR "GLFW source not found at ${CMAKE_CURRENT_SOURCE_DIR}/glfw")
endif ()
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
# Make sure GLFW installs its headers correctly if needed, or rely on source dir
add_subdirectory(glfw) # Creates target 'glfw'

# --- spdlog (Header-Only INTERFACE Target) ---
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/spdlog/include/spdlog/spdlog.h")
    message(FATAL_ERROR "spdlog headers not found at ${CMAKE_CURRENT_SOURCE_DIR}/spdlog/include")
endif()

# Create an INTERFACE library target for spdlog
add_library(spdlog INTERFACE) # Could also name it spdlog::spdlog if preferred

# Specify the include directory needed by targets that link against 'spdlog'
target_include_directories(spdlog INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/spdlog/include # Path relative to ext/CMakeLists.txt
)

# --- Eigen3 ---
# Eigen3 is header-only, so we can just add its include directory.
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/eigen/CMakeLists.txt")
    add_subdirectory(eigen) # Assuming this creates target 'Eigen3::Eigen'
else ()
    message(WARNING "Eigen3 CMakeLists.txt not found. Add include directory manually if needed.")
endif ()

# --- GLM ---
# GLM is often header-only. If its CMakeLists.txt just defines an INTERFACE library,
# add_subdirectory is enough. If it builds something, use that target.
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/glm/CMakeLists.txt")
    add_subdirectory(glm) # Assuming this creates target 'glm' (likely INTERFACE)
    # If no CMakeLists.txt, handle manually in main CMakeLists.txt by adding include dir
else ()
    message(WARNING "GLM CMakeLists.txt not found. Add include directory manually if needed.")
endif ()

# --- EnTT ---
# EnTT is mostly header-only but might have a CMake target for easier consumption.
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/entt/CMakeLists.txt")
    # EnTT's CMake might require specific options
    add_subdirectory(entt) # Assuming this creates target 'EnTT'
else ()
    message(WARNING "EnTT CMakeLists.txt not found. Add include directory manually if needed.")
endif ()


# --- TinyObjLoader ---
# TinyObjLoader is often treated as header-only with one implementation file.
# If it has a CMakeLists.txt that creates a target, use it.
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tinyobjloader/CMakeLists.txt")
    add_subdirectory(tinyobjloader) # Assuming this creates target 'tinyobjloader'
else ()
    # If no CMakeLists.txt, main project includes header, defines implementation once,
    # and adds include directory. No target needed here.
    message(STATUS "TinyObjLoader CMakeLists.txt not found. Assuming header-only usage.")
endif ()


# --- Slang ---
# Slang MUST be built or provide pre-built libraries/headers.
# If you have source and a CMakeLists.txt:
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/slang/CMakeLists.txt")
    # Configure Slang build if necessary (e.g., disable tests)
    # set(SLANG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    add_subdirectory(slang) # Assuming this creates target 'slang'
    # Make sure the 'slang' target exports its include directory and library location.
    # If using pre-built Slang (like your original approach):
    # This block should NOT be here. Finding pre-built Slang belongs in the main CMakeLists.txt
    # or a dedicated FindSlang.cmake module. Let's assume you build it from source via add_subdirectory.
else ()
    message(FATAL_ERROR "Slang CMakeLists.txt not found in ext/slang. Cannot build from source.")
endif ()


# --- Dear ImGui (Static Library) ---
if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.h")
    message(FATAL_ERROR "Dear ImGui headers not found at ${CMAKE_CURRENT_SOURCE_DIR}/imgui")
endif ()
set(IMGUI_DIR imgui) # Relative path within ext/
set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)
# Create a static library for ImGui linked against GLFW
add_library(bcg_imgui STATIC ${IMGUI_SOURCES})
# Specify include directories needed to *build* bcg_imgui and for *users* of bcg_imgui
target_include_directories(bcg_imgui PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/${IMGUI_DIR}          # ImGui root for imgui.h
        ${CMAKE_CURRENT_SOURCE_DIR}/${IMGUI_DIR}/backends # For backend headers
        # GLFW include dir is needed to build imgui_impl_glfw.cpp
        # It should be available via the 'glfw' target's interface includes.
)
# Link bcg_imgui against the glfw target. The PUBLIC keyword ensures that
# targets linking bcg_imgui also implicitly link glfw.
target_link_libraries(bcg_imgui PUBLIC glfw)
message(STATUS "Created bcg_imgui static library target.")

# --- No linking here ---
# Linking happens in the main project that consumes these targets.
# target_link_libraries(${PROJECT_NAME} PRIVATE ...) <- Remove this from ext/CMakeLists.txt