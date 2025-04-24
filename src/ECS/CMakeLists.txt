target_include_directories(${PROJECT_NAME} PUBLIC .)
target_sources(${PROJECT_NAME} PRIVATE
        AABBSystem.cpp
        AABBUtils.cpp
        AssetManager.cpp
)