cmake_minimum_required(VERSION 3.13)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

get_filename_component(DIR_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${DIR_NAME})

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SRC)

add_executable(${PROJECT_NAME} ${SRC})

# target_include_directories(${PROJECT_NAME} PRIVATE)

# target_link_directories(${PROJECT_NAME} PRIVATE)

# target_link_options(${PROJECT_NAME} PRIVATE)

target_link_libraries(${PROJECT_NAME}
    GLESv2
    EGL
    X11
)