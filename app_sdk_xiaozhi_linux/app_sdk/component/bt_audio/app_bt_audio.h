#ifndef APP_BT_AUDIO_H
#define APP_BT_AUDIO_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifndef BT_LOGI
#define BT_LOGI(fmt, ...) printf("[I][%s:%d] " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void (*on_adapter_on)(const char *addr, const char *alias);
    void (*on_adapter_off)(void);

    void (*on_conn_state)(const char *addr, int state);
    void (*on_audio_state)(const char *addr, int state);

    void (*on_play_state)(const char *addr, int play_state);
    void (*on_play_pos)(const char *addr, int song_len_ms, int song_pos_ms);

    void (*on_track)(
        const char *addr,
        const char *title,
        const char *artist,
        const char *album,
        const char *track_num,
        const char *num_tracks,
        const char *genre,
        int duration_ms);

    void (*on_volume)(const char *addr, unsigned int vol_0_127);
} app_bt_audio_observer_t;

int  app_bt_audio_init(const char *alias, const app_bt_audio_observer_t *obs);
void app_bt_audio_deinit(void);
int  app_bt_audio_init_async(const char *alias, const app_bt_audio_observer_t *obs);
int  app_bt_audio_start(void);
int  app_bt_audio_stop(void);
void app_bt_audio_set_discoverable(bool on);
void app_bt_audio_set_observer(const app_bt_audio_observer_t *obs);
void app_bt_audio_clear_observer(void);

#ifdef __cplusplus
}
#endif
#endif
