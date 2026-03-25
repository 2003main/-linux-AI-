cmake_minimum_required(VERSION 3.15)

project(app_control_center)

aux_source_directory(. SOURCES)

add_executable(app_control_center ${SOURCES})

target_include_directories(app_control_center PRIVATE
                                ./
                                ${CMAKE_SOURCE_DIR}/component/font
                                ${CMAKE_SOURCE_DIR}/lvgl
                                ${CMAKE_SOURCE_DIR}/lvgl/demos
                                ${CMAKE_SOURCE_DIR}/component/osal
                                ${CMAKE_SOURCE_DIR}/component/player
                                )


target_link_libraries(app_control_center PRIVATE 
                           lvgl_demos
                           lvgl
                           font
                           player
                           osal
                           asound 
                           pthread 
                           opus 
                           curl
                           boost_system
                           ssl
                           crypto
                           nghttp2
                           )



