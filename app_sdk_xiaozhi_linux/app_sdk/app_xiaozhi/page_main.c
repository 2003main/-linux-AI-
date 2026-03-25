#include <stdio.h>
#include "lvgl.h"
#include "image_conf.h"
#include "font_conf.h"


typedef struct _lv_100ask_xz_ai
{
    lv_obj_t *state_bar_img_wifi;
    lv_obj_t *state_bar_label_state;
    lv_obj_t *state_bar_img_battery;
    lv_obj_t *img_emoji;
    lv_obj_t *label_chat;
} T_lv_100ask_xz_ai, *PT_lv_100ask_xz_ai;

static T_lv_100ask_xz_ai g_pt_lv_100ask_xz_ai;

//声明通用样式
static lv_style_t com_style;
//初始化通用样式
static void com_style_init(){
    //初始化样式
    lv_style_init(&com_style);
    //判断如果样式非空，那就先重置，再设置
    if(lv_style_is_empty(&com_style) == false)
        lv_style_reset(&com_style);
    //样式背景设置为黑色，圆角设置为0，边框宽度设置为0，填充区域设置为0
    lv_style_set_bg_color(&com_style,lv_color_hex(0x000000));
    lv_style_set_radius(&com_style,0);
    lv_style_set_border_width(&com_style,0);
    lv_style_set_pad_all(&com_style,0);
    lv_style_set_outline_width(&com_style,0);
}

//封装字库获取函数
static void obj_font_set(lv_obj_t *obj, int type, uint16_t weight){
    lv_font_t* font = get_font(type, weight);
    if(font != NULL)
        lv_obj_set_style_text_font(obj, font, LV_PART_MAIN);
}

static void delete_current_page(lv_style_t *style){
    lv_obj_t * act_scr = lv_scr_act();
    lv_disp_t * d = lv_obj_get_disp(act_scr);
	if (d->prev_scr == NULL && (d->scr_to_load == NULL || d->scr_to_load == act_scr))
	{
		lv_obj_clean(act_scr);
        lv_style_reset(style);
	}
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int type;
    char text[256];
} UI_Update_Param_t;

static void _ui_update_cb(void *param)
{
    UI_Update_Param_t *p = (UI_Update_Param_t *)param;
    if (!p) return;
    if(p->type == 0)
        lv_label_set_text(g_pt_lv_100ask_xz_ai.state_bar_label_state, p->text);
    else if(p->type == 1)
        lv_label_set_text(g_pt_lv_100ask_xz_ai.label_chat, p->text);
    else if(p->type == 2)
        lv_img_set_src(g_pt_lv_100ask_xz_ai.img_emoji, p->text);
    if (p) {
        free(p);
    }
}

void SetStateString(const char *str)
{
    UI_Update_Param_t *param = (UI_Update_Param_t *)malloc(sizeof(UI_Update_Param_t));
    param->type = 0;
    strncpy(param->text, str, sizeof(param->text) - 1);
    lv_async_call(_ui_update_cb, param);
}

void SetText(const char *str)
{
    UI_Update_Param_t *param = (UI_Update_Param_t *)malloc(sizeof(UI_Update_Param_t));
    param->type = 1;
    strncpy(param->text, str, sizeof(param->text) - 1);
    lv_async_call(_ui_update_cb, param);
}

void SetEmotion(const char *jpgFile)
{
    UI_Update_Param_t *param = (UI_Update_Param_t *)malloc(sizeof(UI_Update_Param_t));
    param->type = 2;
    strncpy(param->text, jpgFile, sizeof(param->text) - 1);
    lv_async_call(_ui_update_cb, param);
}


void OnClicked(void)
{
    static uint16_t index = 0;
    static char *str[][3] = {
        {"待命", "聆听", "回答"},
        {"现在是待命状态哦。", "现在是聆听状态哦。", "现在是回答状态哦。"},
        {GET_IMAGE_PATH("img_joke.png"), GET_IMAGE_PATH("img_naughty.png"), GET_IMAGE_PATH("img_think.png")},
    };
    // lv_label_set_text(g_pt_lv_100ask_xz_ai.state_bar_label_state, str[0][index]);
    // lv_label_set_text(g_pt_lv_100ask_xz_ai.label_chat, str[1][index]);
    // lv_img_set_src(g_pt_lv_100ask_xz_ai.img_emoji, str[2][index]);
    if (index >= 2)
        index = 0;
    else
        index++;

    LV_LOG_USER("Clicked, index: %d", index);
}

static void screen_onclicked_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        OnClicked();
    }
}

void init_page_main(){
    com_style_init();
    lv_obj_t * cont = lv_obj_create(lv_scr_act());
    lv_obj_set_size(cont,LV_PCT(100),LV_PCT(100));
    lv_obj_add_style(cont,&com_style,LV_PART_MAIN);

    /* state bar */
    lv_obj_t *cont_state_bar = lv_obj_create(cont);
    lv_obj_set_size(cont_state_bar, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_add_style(cont_state_bar, &com_style, 0);
    lv_obj_set_align(cont_state_bar,LV_ALIGN_TOP_MID);
    lv_obj_set_layout(cont_state_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont_state_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont_state_bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // wifi
    lv_obj_t *wifi_label = lv_label_create(cont_state_bar);
    lv_label_set_text(wifi_label, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(wifi_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_pad_left(wifi_label,20,0);
    
    // state
    g_pt_lv_100ask_xz_ai.state_bar_label_state = lv_label_create(cont_state_bar);
    obj_font_set(g_pt_lv_100ask_xz_ai.state_bar_label_state,FONT_TYPE_CN, 24);
    lv_obj_center(g_pt_lv_100ask_xz_ai.state_bar_label_state);
    lv_obj_set_style_text_color(g_pt_lv_100ask_xz_ai.state_bar_label_state,lv_color_hex(0xFFFFFF),0);
    lv_label_set_text(g_pt_lv_100ask_xz_ai.state_bar_label_state, "待命");

    // battery
    lv_obj_t *battery_label = lv_label_create(cont_state_bar);
    lv_label_set_text(battery_label, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_color(battery_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_pad_right(battery_label,20,0);

    /* emoji */
    // https://www.iconfont.cn/search/index?searchType=icon&q=%E5%9C%86%E8%84%B8%E8%A1%A8%E6%83%85
    g_pt_lv_100ask_xz_ai.img_emoji = lv_img_create(cont);
    // lv_img_set_src(g_pt_lv_100ask_xz_ai.img_emoji,GET_IMAGE_PATH("img_naughty.png"));
    lv_obj_align(g_pt_lv_100ask_xz_ai.img_emoji, LV_ALIGN_CENTER, 0, -40);

    /* chat */
    g_pt_lv_100ask_xz_ai.label_chat = lv_label_create(cont);
    lv_obj_center(g_pt_lv_100ask_xz_ai.label_chat);
    lv_obj_set_style_text_color(g_pt_lv_100ask_xz_ai.label_chat,lv_color_hex(0xFFFFFF),0);
    obj_font_set(g_pt_lv_100ask_xz_ai.label_chat,FONT_TYPE_CN, 20);
    lv_label_set_text(g_pt_lv_100ask_xz_ai.label_chat, "Hi！有什么可以帮到你呢？");
    lv_obj_align_to(g_pt_lv_100ask_xz_ai.label_chat, g_pt_lv_100ask_xz_ai.img_emoji, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    // screen touch
    lv_obj_add_flag(lv_layer_top(), LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(lv_layer_top(), screen_onclicked_event_cb, LV_EVENT_CLICKED, NULL);
}