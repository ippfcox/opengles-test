cmake_minimum_required(VERSION 3.13)

get_filename_component(DIR_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${DIR_NAME})

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/../common SRC)

add_executable(${PROJECT_NAME} ${SRC})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../common
)

# target_link_directories(${PROJECT_NAME} PRIVATE)

# target_link_options(${PROJECT_NAME} PRIVATE)

target_link_libraries(${PROJECT_NAME}
    GLESv2
    EGL
    X11
)