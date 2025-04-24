target_include_directories(${PROJECT_NAME} PUBLIC .)
target_sources(${PROJECT_NAME} PRIVATE
        ShaderManager.cpp
        ShaderData.cpp
)