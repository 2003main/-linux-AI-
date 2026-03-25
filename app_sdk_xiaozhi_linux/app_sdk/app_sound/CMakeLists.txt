cmake_minimum_required(VERSION 3.15)

project(app_sound)

aux_source_directory(. SOURCES)

add_executable(app_sound ${SOURCES})

target_include_directories(app_sound PRIVATE
                                ./
                                ${CMAKE_SOURCE_DIR}/component/font
                                ${CMAKE_SOURCE_DIR}/lvgl
                                ${CMAKE_SOURCE_DIR}/lvgl/demos
                                ${CMAKE_SOURCE_DIR}/component/osal
                                ${CMAKE_SOURCE_DIR}/component/player
                                )


target_link_libraries(app_sound PRIVATE 
                           lvgl_demos
                           lvgl
                           font
                           player
                           osal
                           asound 
                           pthread 
                           opus 
                           speexdsp
                           )



