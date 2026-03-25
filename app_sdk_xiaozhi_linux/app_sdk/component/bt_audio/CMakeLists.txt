# 蓝牙音频组件
cmake_minimum_required(VERSION 3.15)

set(BT_AUDIO_DIR ${CMAKE_CURRENT_LIST_DIR})
set(BT_AUDIO_INC ${BT_AUDIO_DIR}/include)
set(BT_AUDIO_LIB ${BT_AUDIO_DIR}/lib)

# 添加头文件
include_directories(${BT_AUDIO_INC})

# 添加源文件
file(GLOB BT_AUDIO_SRCS
    ${BT_AUDIO_DIR}/app_bt_audio.c
)

add_library(bt_audio STATIC ${BT_AUDIO_SRCS})

if(SIMULATOR_LINUX)

else()
    # 链接库路径
    target_link_directories(bt_audio PUBLIC
        ${BT_AUDIO_LIB}
    )
    # 链接依赖库
    target_link_libraries(bt_audio
        btmg  #Tina Linux 中的蓝牙管理库，提供连接、配对、设备发现等高层蓝牙功能。
        wirelesscom #与通信管理相关，用于配置网络连接、扫描等。
        shared-mainloop #用于集成多个事件源（如DBus事件、蓝牙事件）到一个统一的主循环中。
        bluetooth-internal #从命名看，可能是蓝牙协议栈的内部底层接口或工具库，为上层（如btmg）提供核心支持。
        gio-2.0 #GLib 库中的一部分，提供高级抽象API，用于处理网络、文件系统、进程通信等I/O操作。
        dbus-1 #进程间通信（IPC） 系统总线，允许应用程序（如蓝牙服务、网络管理器）相互通信和交换消息。
        asound #ALSA（高级Linux声音架构）的库，提供音频设备接口，用于播放、录制和处理音频数据。
        json-c #一个C语言实现的JSON解析和构建库，用于处理配置文件和数据序列化。
        # gmodule-2.0
        # bluetooth
        # sbc
        # glib-2.0
        # gobject-2.0
        # ffi z json-c asound readline ncurses
        # dl pthread m resolv
    )
endif()
