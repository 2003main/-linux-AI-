
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "lvgl.h"
#include "font_conf.h"
#include "image_conf.h"
#include "osal_conf.h"
#include "osal_thread.h"
#include "music_conf.h"
#include "ui_system.h"
#include "page_main.h"

extern void lv_port_disp_init(bool is_disp_orientation);
extern void lv_port_indev_init(void);

static void obj_font_set(lv_obj_t *obj,int type, uint16_t weight){
    lv_font_t* font = get_font(type, weight);
    if(font != NULL)
        lv_obj_set_style_text_font(obj, font, 0);
}

int main() {
    lv_init();
    lv_port_disp_init(true);
    lv_port_indev_init();
    font_init();
    ui_system_init();
    init_page_main();

    while (1) {
        lv_task_handler();
		usleep(1000);
    }
    return 0;
}
