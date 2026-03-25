#ifndef _RES_CONF_H_
#define _RES_CONF_H_

#ifdef SIMULATOR_LINUX
    #define FONT_PATH "./build/app1/res/font/"
    #define IMAGE_PATH "./build/app1/res/image/"
    #define MUSIC_PATH "./build/app1/res/music/"
    #define SAVE_PATH "./build/app1/res/"
#else
    #define FONT_PATH "/usr/res/font/"
    #define IMAGE_PATH "/usr/res/image/"
    #define MUSIC_PATH "/usr/res/music/"
    #define SAVE_PATH "/usr/res/"
#endif

#endif