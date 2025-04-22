cmake_minimum_required(VERSION 3.16)

project(VulkanEnTTApp LANGUAGES CXX)

# --- Project Settings ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

# --- Find Vulkan SDK ---
# (Keep your existing Vulkan SDK finding logic - find_package(Vulkan REQUIRED))
find_package(Vulkan REQUIRED)
if(NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan SDK not found...") # Shortened for brevity
else()
    message(STATUS "Found Vulkan: ${Vulkan_INCLUDE_DIRS} | ${Vulkan_LIBRARIES}")
endif()

# --- Find CUDA Toolkit ---
# (Keep your existing CUDA Toolkit finding logic - find_package(CUDAToolkit REQUIRED))
find_package(CUDAToolkit REQUIRED)
if(NOT CUDAToolkit_FOUND)
    message(FATAL_ERROR "CUDA Toolkit not found...") # Shortened for brevity
else()
    message(STATUS "Found CUDAToolkit: ${CUDAToolkit_INCLUDE_DIRS} | Version: ${CUDAToolkit_VERSION}")
endif()

# --- Make executable ---
add_executable(${PROJECT_NAME} src/main.cpp)

# --- Process External Dependencies ---
add_subdirectory(ext)

# --- Process Source Files ---
add_subdirectory(src)

# --- Define Executable Target ---
target_sources(${PROJECT_NAME} PRIVATE
        src/Application/Application.cpp
        src/Camera/CameraSystem.cpp
        src/Camera/CameraUtils.cpp
        src/Core/Logger.cpp
        src/Core/InputManager.cpp
        src/Core/WindowManager.cpp
        src/ECS/TransformSystem.cpp
        src/ECS/AABBSystem.cpp
        src/ECS/TransformUtils.cpp
        src/Rendering/RendererSystem.cpp
        src/Rendering/ShaderData.cpp
        src/Rendering/VulkanUtils.cpp
        src/Rendering/VulkanContext.cpp
        src/Rendering/VulkanWrappers.cpp
        src/Scene/SceneManager.cpp
        src/UI/UIManager.cpp
)

# --- Set Include Directories ---
target_include_directories(${PROJECT_NAME} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src # Assuming your source files are now in src/
        # Add subdirectories within src/ if you structured it that way
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Application
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Rendering
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Platform
        ${CMAKE_CURRENT_SOURCE_DIR}/src/UI
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Input
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Camera
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Scene
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ECS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/Core # If you created this
        # Vulkan and CUDA includes are usually propagated by find_package targets
        # External library includes should be propagated by their targets (see ext/CMakeLists.txt)
        # If not propagated automatically, add them back:
        # ${EXT_DIR}/imgui
        # ${EXT_DIR}/imgui/backends
        # ${EXT_DIR}/glm             # Needed if target doesn't provide INTERFACE_INCLUDE_DIRECTORIES
        # ${EXT_DIR}/entt/src        # Needed if target doesn't provide INTERFACE_INCLUDE_DIRECTORIES
        # ${EXT_DIR}/tinyobjloader   # Needed if target doesn't provide INTERFACE_INCLUDE_DIRECTORIES
        # ${EXT_DIR}/slang/include   # Needed if target doesn't provide INTERFACE_INCLUDE_DIRECTORIES
)
target_include_directories(${PROJECT_NAME} PUBLIC # Or PRIVATE
        # ... other includes ...

)

# --- Link Libraries ---
target_link_libraries(${PROJECT_NAME} PRIVATE
        # Vulkan and CUDA targets (from find_package)
        Vulkan::Vulkan
        CUDA::cudart

        # Targets created in ext/CMakeLists.txt
        glfw            # Target name defined by GLFW's CMake (usually 'glfw')
        slang           # Assuming slang's CMake creates a 'slang' target
        EnTT            # Assuming entt's CMake creates an 'EnTT' target
        tinyobjloader   # Assuming tinyobjloader's CMake creates a 'tinyobjloader' target
        glm             # Assuming glm's CMake creates a 'glm' target (might be INTERFACE)
        bcg_imgui       # The static library target you created for ImGui
        Eigen3::Eigen   # Assuming Eigen's CMake creates an 'Eigen3::Eigen' target
        spdlog          # Assuming spdlog's CMake creates a 'spdlog' target
)

# --- Copy Assets ---
# (Keep your asset copying logic)
# ...
file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders)
file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/models)

# Check if source directories exist
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/")
    message(WARNING "Source directory for shaders not found: ${CMAKE_CURRENT_SOURCE_DIR}/shaders/")
endif()
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/models/")
    message(WARNING "Source directory for models not found: ${CMAKE_CURRENT_SOURCE_DIR}/models/")
endif()

# Copy shader files from local source shaders/ dir
# Use file GLOB to find files (or list them explicitly)
file(GLOB SHADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.slang")
if(SHADER_FILES)
    message(STATUS "Copying shaders: ${SHADER_FILES}")
    file(COPY ${SHADER_FILES}
            DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders)
else()
    message(WARNING "No .slang files found in ${CMAKE_CURRENT_SOURCE_DIR}/shaders/ to copy.")
endif()


# Copy model files from local source models/ dir
# Use file GLOB to find files (or list them explicitly)
file(GLOB MODEL_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/models/*.obj"
        "${CMAKE_CURRENT_SOURCE_DIR}/models/*.mtl"
)
if(MODEL_FILES)
    message(STATUS "Copying models: ${MODEL_FILES}")
    file(COPY ${MODEL_FILES}
            DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/models/)
else()
    message(WARNING "No .obj or .mtl files found in ${CMAKE_CURRENT_SOURCE_DIR}/models/ to copy.")
endif()

message(STATUS "Build configured...")