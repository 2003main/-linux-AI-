/*
 * @Author: xiaozhi
 * @Date: 2025-10-18
 * @File: page_bt_audio.c
 * @Description: 蓝牙音频页面（歌名+专辑 + 歌词 + 时间显示）
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "lvgl.h"
#include "color_conf.h"
#include "font_conf.h"
#include "image_conf.h"
#include "page_conf.h"

#if (SIMULATOR_LINUX == t113)
#include "app_bt_audio.h"
#endif

static int g_bt_audio_running = 0;

typedef struct {
    char title[128];    // 歌词内容（绿色）
    char artist[128];   // 歌名
    char album[128];    // 专辑
    int volume;         // 0~127
    int play_state;     // 0=暂停, 1=播放
    int progress;       // 0~100
    int pos_ms;         // 当前毫秒
    int len_ms;         // 总时长
} bt_audio_info_t;

static bt_audio_info_t g_bt_info = {0};

/* ---------- UI 对象 ---------- */
static lv_style_t com_style;
static lv_obj_t *label_title, *label_album, *label_lyric;
static lv_obj_t *bar_progress, *label_time;
static lv_obj_t *volume_bar, *label_volume;
static lv_obj_t *icon_play;

/* ---------- 工具 ---------- */
static void com_style_init()
{
    lv_style_init(&com_style);
    if (!lv_style_is_empty(&com_style))
        lv_style_reset(&com_style);
    lv_style_set_bg_color(&com_style,APP_COLOR_BLACK);
    lv_style_set_radius(&com_style,0);
    lv_style_set_border_width(&com_style,0);
    lv_style_set_pad_all(&com_style,0);
}

static void obj_font_set(lv_obj_t *obj, int type, uint16_t weight)
{
    lv_font_t *font = get_font(type, weight);
    if (font)
        lv_obj_set_style_text_font(obj, font, 0);
}

/* ---------- 返回按钮 ---------- */
static void back_btn_click_event_cb(lv_event_t *e)
{
    g_bt_audio_running = 0;
    delete_current_page(&com_style);
    init_page_main();
}

/* ---------- 顶部标题栏 ---------- */
static lv_obj_t *init_title_view(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, LV_PCT(100), 60);
    lv_obj_add_style(cont, &com_style, 0);
    lv_obj_set_style_bg_color(cont, APP_COLOR_WHITE, 0);
    lv_obj_set_style_bg_opa(cont,LV_OPA_0,LV_PART_MAIN);


    lv_obj_t *btn = lv_btn_create(cont);
    lv_obj_set_size(btn, 80, 60);
    lv_obj_clear_state(btn,LV_STATE_FOCUS_KEY);
    lv_obj_set_style_border_width(btn, 0,0);
    lv_obj_set_style_shadow_width(btn, 0,0);
    lv_obj_set_style_radius(btn,35,0);
    lv_obj_set_style_bg_color(btn,APP_COLOR_BUTTON_DEFALUT,0);
    lv_obj_align(btn, LV_ALIGN_LEFT_MID, 20, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_0, 0);   // 透明背景
    lv_obj_add_event_cb(btn, back_btn_click_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *icon = lv_img_create(btn);
    lv_img_set_src(icon, GET_IMAGE_PATH("icon_back.png"));
    lv_obj_center(icon);

    lv_obj_t *title = lv_label_create(cont);
    obj_font_set(title, FONT_TYPE_CN, FONT_SIZE_TITLE_2);
    lv_label_set_text(title, "蓝牙音箱");
    lv_obj_set_style_text_color(title, APP_COLOR_WHITE, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    return cont;
}

/* ---------- 左侧音量条 ---------- */
static lv_obj_t *init_volume_bar(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_add_style(cont, &com_style, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_0, 0);
    lv_obj_set_size(cont, 50, 220);
    lv_obj_align(cont, LV_ALIGN_RIGHT_MID, -30, 0);   // ✅ 靠右侧显示


    volume_bar = lv_bar_create(cont);
    lv_obj_set_size(volume_bar, 20, 200);
    lv_obj_center(volume_bar);
    lv_bar_set_range(volume_bar, 0, 127);
    lv_bar_set_value(volume_bar, g_bt_info.volume, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(volume_bar, APP_COLOR_GRAY, 0);
    lv_obj_set_style_bg_color(volume_bar, APP_COLOR_BUTTON_DEFALUT, LV_PART_INDICATOR);
    lv_obj_set_style_radius(volume_bar, 8, 0);

    label_volume = lv_label_create(cont);
    obj_font_set(label_volume, FONT_TYPE_LETTER, FONT_SIZE_TEXT_1);
    lv_obj_set_style_text_color(label_volume, APP_COLOR_WHITE, 0);
    lv_label_set_text_fmt(label_volume, "%d", g_bt_info.volume);
    lv_obj_align_to(label_volume, volume_bar, LV_ALIGN_OUT_TOP_MID, 0, -8);

    return cont;
}

/* ---------- 中部信息 ---------- */
static lv_obj_t *init_info_view(lv_obj_t *parent)
{
    // 信息容器部分（歌名 + 专辑）
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_add_style(cont, &com_style, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_0, 0);
    lv_obj_set_size(cont, LV_PCT(100), 360);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 40); // 稍微靠上，留出歌词区域

    /* 第一行：歌名 + 专辑名 */
    lv_obj_t *line_cont = lv_obj_create(cont);
    lv_obj_add_style(line_cont, &com_style, 0);
    lv_obj_set_style_bg_opa(line_cont, LV_OPA_0, 0);
    lv_obj_set_size(line_cont, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_align(line_cont, LV_ALIGN_TOP_MID, 0, 20);  // ✅ 向下移动一点
    lv_obj_set_flex_flow(line_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(line_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // 歌名
    label_title = lv_label_create(line_cont);
    obj_font_set(label_title, FONT_TYPE_CN, FONT_SIZE_TEXT_1);
    lv_obj_set_style_text_color(label_title, APP_COLOR_WHITE, 0);
    lv_label_set_text(label_title, "未知歌曲");

    // 分隔符
    lv_obj_t *sp = lv_label_create(line_cont);
    lv_label_set_text(sp, " - ");
    lv_obj_set_style_text_color(sp, APP_COLOR_GRAY, 0);

    // 专辑
    label_album = lv_label_create(line_cont);
    obj_font_set(label_album, FONT_TYPE_CN, FONT_SIZE_TEXT_1);
    lv_obj_set_style_text_color(label_album, APP_COLOR_GRAY, 0);
    lv_label_set_text(label_album, "未知专辑");

    /* 绿色歌词（放到主界面层上） */
    label_lyric = lv_label_create(parent);
    obj_font_set(label_lyric, FONT_TYPE_CN, FONT_SIZE_TIME_1); // 🚀 字体超大号
    lv_obj_set_style_text_color(label_lyric, APP_COLOR_WHITE, 0);
    lv_label_set_text(label_lyric, "歌词加载中...");

    // 向上移动歌词位置（屏幕中心偏上）
    lv_obj_align(label_lyric, LV_ALIGN_CENTER, 0, 0);

    return cont;
}

/* ---------- 底部进度条 + 时间显示 ---------- */
static lv_obj_t *init_bottom_control(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, LV_PCT(100), 100);
    lv_obj_add_style(cont, &com_style, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_0, 0);
    lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, 0);

    /* 使用 Flex 水平布局，让内容在底部居中排列 */
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont,
                          LV_FLEX_ALIGN_CENTER,  // 主轴居中（水平）
                          LV_FLEX_ALIGN_CENTER,  // 交叉轴居中（垂直）
                          LV_FLEX_ALIGN_CENTER); // 内容居中

    /* 播放/暂停图标（左侧） */
    icon_play = lv_img_create(cont);
    lv_img_set_src(icon_play, LV_SYMBOL_PLAY);
    lv_obj_set_style_text_color(icon_play, APP_COLOR_WHITE, 0); // 白色
    lv_obj_set_size(icon_play, 36, 36);

    // ✅ 手动微调位置：稍往下、靠右
    lv_obj_set_style_translate_y(icon_play, 10, 0);   // 向下 6px
    lv_obj_set_style_pad_right(icon_play, 0, 0);     // 与进度条更贴近（原10改为4）

    /* 进度条 */
    bar_progress = lv_bar_create(cont);
    lv_obj_set_size(bar_progress, 420, 14);
    lv_bar_set_range(bar_progress, 0, 100);
    lv_bar_set_value(bar_progress, g_bt_info.progress, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar_progress, APP_COLOR_GRAY, 0);
    lv_obj_set_style_bg_color(bar_progress, APP_COLOR_BUTTON_DEFALUT, LV_PART_INDICATOR);
    lv_obj_set_style_pad_right(bar_progress, 0, 0);  // 与时间文字保持合适间距

    /* 时间显示 */
    label_time = lv_label_create(cont);
    obj_font_set(label_time, FONT_TYPE_LETTER, FONT_SIZE_TEXT_1);
    lv_obj_set_style_text_color(label_time, APP_COLOR_WHITE, 0);
    lv_label_set_text(label_time, "0:00 / 0:00");

    return cont;
}

/* ---------- 蓝牙回调 ---------- */
static void format_time_str(char *buf, size_t len, int ms)
{
    int total_sec = ms / 1000;
    int min = total_sec / 60;
    int sec = total_sec % 60;
    snprintf(buf, len, "%d:%02d", min, sec);
}

static void _ui_update_cb(void *p)
{
    bt_audio_info_t *info = (bt_audio_info_t *)p;
    if (!info) return;

    char cur[16], total[16];
    format_time_str(cur, sizeof(cur), info->pos_ms);
    format_time_str(total, sizeof(total), info->len_ms);

    lv_label_set_text(label_title, info->artist);
    lv_label_set_text_fmt(label_album, "%s", info->album);
    lv_label_set_text(label_lyric, info->title); // 歌词绿色大字

    lv_bar_set_value(bar_progress, info->progress, LV_ANIM_OFF);
    lv_label_set_text_fmt(label_time, "%s / %s", cur, total);

    lv_bar_set_value(volume_bar, info->volume, LV_ANIM_OFF);
    lv_label_set_text_fmt(label_volume, "%d", info->volume);

    free(info);
}

#if (SIMULATOR_LINUX == t113)

/* ---------- 播放状态 UI 更新回调 ---------- */
static void _update_play_icon_cb(void *p)
{
    int state = (int)(intptr_t)p;
    printf("==> async update play_state:%d\r\n", state);

    if (state == 1)
        lv_img_set_src(icon_play, LV_SYMBOL_PAUSE);   // 播放中 → 显示“暂停”
    else if (state == 2)
        lv_img_set_src(icon_play, LV_SYMBOL_PLAY);    // 暂停 → 显示“播放”
    else
        lv_img_set_src(icon_play, LV_SYMBOL_STOP);    // 其他状态备用
}

/* ---------- 蓝牙回调 ---------- */
static void ui_on_play_state(const char *addr, int play_state)
{
    (void)addr;
    printf("[UI_PLAY_STATE] recv play_state=%d\n", play_state);

    g_bt_info.play_state = play_state;

    if(!g_bt_audio_running)return;

    // ✅ 异步更新播放图标
    lv_async_call(_update_play_icon_cb, (void*)(intptr_t)play_state);
}

static void ui_on_track(const char *addr, const char *title,
                        const char *artist, const char *album,
                        const char *tn, const char *nt,
                        const char *genre, int duration_ms)
{
    (void)addr; (void)tn; (void)nt; (void)genre;
    snprintf(g_bt_info.title, sizeof(g_bt_info.title), "%s", title ? title : "歌词未知");
    snprintf(g_bt_info.artist, sizeof(g_bt_info.artist), "%s", artist ? artist : "未知歌曲");
    snprintf(g_bt_info.album, sizeof(g_bt_info.album), "%s", album ? album : "未知专辑");
    g_bt_info.len_ms = duration_ms;

    if(!g_bt_audio_running)return;

    bt_audio_info_t *info = malloc(sizeof(bt_audio_info_t));
    if (!info) return;
    memcpy(info, &g_bt_info, sizeof(*info));
    lv_async_call(_ui_update_cb, info);
}

static void ui_on_pos(const char *addr, int len_ms, int pos_ms)
{
    (void)addr;
    g_bt_info.len_ms = len_ms;
    g_bt_info.pos_ms = pos_ms;
    g_bt_info.progress = (len_ms > 0) ? (int)((long long)pos_ms * 100 / len_ms) : 0;

    if(!g_bt_audio_running)return;

    bt_audio_info_t *info = malloc(sizeof(bt_audio_info_t));
    if (!info) return;
    memcpy(info, &g_bt_info, sizeof(*info));
    lv_async_call(_ui_update_cb, info);
}

static void ui_on_volume(const char *addr, unsigned int vol_0_127)
{
    (void)addr;
    if (vol_0_127 > 127) vol_0_127 = 127;
    g_bt_info.volume = vol_0_127;

    if(!g_bt_audio_running)return;

    bt_audio_info_t *info = malloc(sizeof(bt_audio_info_t));
    if (!info) return;
    memcpy(info, &g_bt_info, sizeof(*info));
    lv_async_call(_ui_update_cb, info);
}

/* ---------- 注册 ---------- */
static app_bt_audio_observer_t obs = {
    .on_track      = ui_on_track,
    .on_play_pos   = ui_on_pos,
    .on_volume     = ui_on_volume,
    .on_play_state = ui_on_play_state,
};

#endif

/* ---------- 页面初始化 ---------- */
void init_page_bt_audio(void)
{

#ifndef SIMULATOR_LINUX
    static int init_once = 0;
    if (!init_once)
    {
        app_bt_audio_start();
        app_bt_audio_set_observer(&obs);
        init_once = 1;
    }
#endif

    com_style_init();
    lv_obj_t *root = lv_obj_create(lv_scr_act());
    lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
    lv_obj_add_style(root, &com_style, 0);

    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *bg_img = lv_img_create(root);
    lv_img_set_src(bg_img,GET_IMAGE_PATH("bg_bt_audio.png"));
    // lv_obj_align(bg_img,LV_ALIGN_RIGHT_MID,-2,0);

    init_title_view(root);
    init_volume_bar(root);
    init_info_view(root);
    init_bottom_control(root);

    g_bt_audio_running = 1;
}
