cmake_minimum_required(VERSION 3.15)

project(app_xiaozhi)

# 添加显式定义，定义宏 SIMULATOR_LINUX,这样才能在代码中识别到该宏
if(SIMULATOR_LINUX)
    add_compile_definitions(SIMULATOR_LINUX)  
endif()
# 拷贝res文件夹到build中，方便后续打包
file(COPY res DESTINATION ${PROJECT_BINARY_DIR})

aux_source_directory(. SOURCES)

add_executable(app_xiaozhi ${SOURCES})

target_include_directories(app_xiaozhi PRIVATE
                                ./
                                res/
                                res/font/
                                res/image/
                                res/music/
                                ${CMAKE_SOURCE_DIR}/component/font
                                ${CMAKE_SOURCE_DIR}/lvgl
                                ${CMAKE_SOURCE_DIR}/lvgl/demos
                                ${CMAKE_SOURCE_DIR}/component/osal
                                ${CMAKE_SOURCE_DIR}/component/player
                                ${CMAKE_SOURCE_DIR}/component/net
                                )


target_link_libraries(app_xiaozhi PRIVATE 
                           lvgl_demos
                           lvgl
                           font
                           player
                           osal

                           
                           curl
                           ssl
                           crypto
                           nghttp2
                           http
                           cjson
                           m 
                           pthread
                           )



