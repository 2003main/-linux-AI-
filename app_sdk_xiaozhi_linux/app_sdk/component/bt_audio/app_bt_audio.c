#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <stdlib.h>

#include "app_bt_audio.h"
#include "bt_manager.h"
#include "bt_log.h"

#ifndef BT_LOGI
#define BT_LOGI(fmt, ...) printf("[I][%s:%d] " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#endif

#ifdef SIMULATOR_LINUX

int  app_bt_audio_init(const char *alias, const app_bt_audio_observer_t *obs){return 0;}
void app_bt_audio_deinit(void){}
int  app_bt_audio_init_async(const char *alias, const app_bt_audio_observer_t *obs){return 0;}
int  app_bt_audio_start(void){return 0;}
int  app_bt_audio_stop(void){return 0;}
void app_bt_audio_set_discoverable(bool on){}
void app_bt_audio_set_observer(const app_bt_audio_observer_t *obs){}
void app_bt_audio_clear_observer(void){}

#else

typedef struct {
    char alias[64];
    app_bt_audio_observer_t obs;
    int has_obs;
} _bt_init_args_t;

static volatile int g_shutting_down = 0;
static volatile int g_running = 0;
static btmg_callback_t *g_cb = NULL;
static app_bt_audio_observer_t g_obs;
static pthread_mutex_t g_obs_lock = PTHREAD_MUTEX_INITIALIZER;

static char g_peer_addr[18] = {0};
static int  g_has_peer = 0;

static inline void _obs_snapshot(app_bt_audio_observer_t *dst) {
    pthread_mutex_lock(&g_obs_lock);
    *dst = g_obs;
    pthread_mutex_unlock(&g_obs_lock);
}

void app_bt_audio_set_observer(const app_bt_audio_observer_t *obs)
{
    pthread_mutex_lock(&g_obs_lock);
    if (obs) {
        g_obs = *obs;
    } else {
        memset(&g_obs, 0, sizeof(g_obs));
    }
    pthread_mutex_unlock(&g_obs_lock);
    BT_LOGI("observer %s", obs ? "registered/updated" : "cleared");
}

void app_bt_audio_clear_observer(void)
{
    app_bt_audio_set_observer(NULL);
}

/* ========== 工具函数 ========== */
static void clear_all_callbacks(void)
{
    if (!g_cb) return;

    memset(&g_cb->btmg_adapter_cb,      0, sizeof(g_cb->btmg_adapter_cb));
    memset(&g_cb->btmg_gap_cb,          0, sizeof(g_cb->btmg_gap_cb));
    memset(&g_cb->btmg_agent_cb,        0, sizeof(g_cb->btmg_agent_cb));
    memset(&g_cb->btmg_a2dp_sink_cb,    0, sizeof(g_cb->btmg_a2dp_sink_cb));
    memset(&g_cb->btmg_a2dp_source_cb,  0, sizeof(g_cb->btmg_a2dp_source_cb));
    memset(&g_cb->btmg_avrcp_cb,        0, sizeof(g_cb->btmg_avrcp_cb));
    memset(&g_cb->btmg_hfp_hf_cb,       0, sizeof(g_cb->btmg_hfp_hf_cb));
    memset(&g_cb->btmg_spp_client_cb,   0, sizeof(g_cb->btmg_spp_client_cb));
    memset(&g_cb->btmg_spp_server_cb,   0, sizeof(g_cb->btmg_spp_server_cb));
}

/* T113 SDK 没有公开 stream_cb_enable 接口，直接留空 */
static void disable_stream_callbacks(void) {}

/* ========== 回调函数 ========== */
static void adapter_state_cb(btmg_adapter_state_t status)
{
    if (g_shutting_down) return;

    if (status == BTMG_ADAPTER_ON) {
        char addr[18] = {0};
        char alias[64] = {0};
        bt_manager_get_adapter_address(addr);
        bt_manager_get_adapter_name(alias);
        BT_LOGI("Adapter ON addr=%s alias=%s", addr, alias);

        bt_manager_agent_set_io_capability(BTMG_IO_CAP_NOINPUTNOOUTPUT);
        bt_manager_set_scan_mode(BTMG_SCAN_MODE_NONE);

        if (g_obs.on_adapter_on)
            g_obs.on_adapter_on(addr, alias);
    } else {
        BT_LOGI("Adapter OFF");
        if (g_obs.on_adapter_off)
            g_obs.on_adapter_off();
    }
}

static void a2dp_sink_conn_state_cb(const char *addr, btmg_a2dp_sink_connection_state_t state)
{
    if (g_shutting_down) return;
    BT_LOGI("A2DP conn state %d: %s", (int)state, addr ? addr : "?");

    switch (state) {
    case BTMG_A2DP_SINK_CONNECTED:
        if (addr) {
            strncpy(g_peer_addr, addr, sizeof(g_peer_addr) - 1);
            g_peer_addr[sizeof(g_peer_addr) - 1] = '\0';
            g_has_peer = 1;
        }
        break;
    case BTMG_A2DP_SINK_DISCONNECTED:
        g_peer_addr[0] = '\0';
        g_has_peer = 0;
        bt_manager_set_scan_mode(g_running ? BTMG_SCAN_MODE_CONNECTABLE_DISCOVERABLE
                                           : BTMG_SCAN_MODE_NONE);
        break;
    default:
        break;
    }

    if (g_obs.on_conn_state)
        g_obs.on_conn_state(addr, (int)state);
}

static void a2dp_sink_audio_state_cb(const char *addr, btmg_a2dp_sink_audio_state_t state)
{
    if (g_shutting_down) return;
    BT_LOGI("A2DP audio state %d: %s", (int)state, addr ? addr : "?");

    if (g_obs.on_audio_state)
        g_obs.on_audio_state(addr, (int)state);
}

static void avrcp_play_state_cb(const char *addr, btmg_avrcp_play_state_t state)
{
    if (g_shutting_down) return;
    BT_LOGI("Play state %d: %s", (int)state, addr ? addr : "?");
    if (g_obs.on_play_state)
        g_obs.on_play_state(addr, (int)state);
}

static void avrcp_play_pos_cb(const char *addr, int song_len, int song_pos)
{
    if (g_shutting_down) return;
    BT_LOGI("Pos %d / %d : %s", song_pos, song_len, addr ? addr : "?");
    if (g_obs.on_play_pos)
        g_obs.on_play_pos(addr, song_len, song_pos);
}

static void avrcp_track_changed_cb(const char *addr, btmg_track_info_t info)
{
    if (g_shutting_down) return;
    const char *title  = info.title      ? info.title      : "";
    const char *artist = info.artist     ? info.artist     : "";
    const char *album  = info.album      ? info.album      : "";
    const char *tn     = info.track_num  ? info.track_num  : "";
    const char *nt     = info.num_tracks ? info.num_tracks : "";
    const char *genre  = info.genre      ? info.genre      : "";
    const char *durstr = info.duration   ? info.duration   : "";

    BT_LOGI("TrackChanged: title=\"%s\" artist=\"%s\" album=\"%s\" duration=%s",
            title, artist, album, durstr);

    if (g_obs.on_track)
        g_obs.on_track(addr, title, artist, album, tn, nt, genre, 0);
}

static void avrcp_volume_cb(const char *addr, unsigned int volume)
{
    if (g_shutting_down) return;
    BT_LOGI("Volume %u : %s", volume, addr ? addr : "?");
    if (g_obs.on_volume)
        g_obs.on_volume(addr, volume);
}

/* ========== 初始化流程 ========== */
int app_bt_audio_init(const char *alias, const app_bt_audio_observer_t *obs)
{
    if (g_cb) return 0;

    memset(&g_obs, 0, sizeof(g_obs));
    app_bt_audio_set_observer(obs);

    // bt_manager_set_loglevel(BTMG_LOG_LEVEL_NONE);
    btmg_set_log_file_path("/tmp/btmg.log");

    if (bt_manager_preinit(&g_cb) != 0) {
        BT_LOGI("bt_manager_preinit failed");
        return -1;
    }

    bt_manager_enable_profile(BTMG_A2DP_SINK_ENABLE | BTMG_AVRCP_ENABLE);

    g_cb->btmg_adapter_cb.adapter_state_cb = adapter_state_cb;
    g_cb->btmg_a2dp_sink_cb.a2dp_sink_connection_state_cb = a2dp_sink_conn_state_cb;
    g_cb->btmg_a2dp_sink_cb.a2dp_sink_audio_state_cb = a2dp_sink_audio_state_cb;
    g_cb->btmg_avrcp_cb.avrcp_play_state_cb = avrcp_play_state_cb;
    g_cb->btmg_avrcp_cb.avrcp_play_position_cb = avrcp_play_pos_cb;
    g_cb->btmg_avrcp_cb.avrcp_track_changed_cb = avrcp_track_changed_cb;
    g_cb->btmg_avrcp_cb.avrcp_audio_volume_cb = avrcp_volume_cb;

    if (bt_manager_init(g_cb) != 0) {
        BT_LOGI("bt_manager_init failed");
        return -1;
    }

    bt_manager_enable(true);

    if (alias && alias[0])
        bt_manager_set_adapter_name(alias);

    bt_manager_set_scan_mode(BTMG_SCAN_MODE_NONE);

    g_running = 0;
    g_peer_addr[0] = '\0';
    g_has_peer = 0;
    return 0;
}

/* 线程函数封装 */
static void *bt_audio_init_process(void *arg)
{
    _bt_init_args_t *a = (_bt_init_args_t *)arg;
    const char *alias = (a && a->alias[0]) ? a->alias : NULL;
    const app_bt_audio_observer_t *obs = (a && a->has_obs) ? &a->obs : NULL;
    app_bt_audio_init(alias, obs);
    if (a) free(a);
    return NULL;
}

int app_bt_audio_init_async(const char *alias, const app_bt_audio_observer_t *obs)
{
    pthread_t th;
    _bt_init_args_t *a = (_bt_init_args_t *)calloc(1, sizeof(*a));
    if (!a) return -1;

    if (alias) snprintf(a->alias, sizeof(a->alias), "%s", alias);
    if (obs) {
        a->obs = *obs;
        a->has_obs = 1;
    }

    if (pthread_create(&th, NULL, bt_audio_init_process, a) != 0) {
        perror("pthread_create");
        free(a);
        return -1;
    }
    pthread_detach(th);
    return 0;
}

int app_bt_audio_start(void)
{
    if (!g_cb) return -1;
    if (g_running) return 0;

    bt_manager_set_scan_mode(BTMG_SCAN_MODE_CONNECTABLE_DISCOVERABLE);
    g_running = 1;
    BT_LOGI("audio start: connectable + discoverable");
    return 0;
}

int app_bt_audio_stop(void)
{
    if (!g_cb) return -1;

    if (g_has_peer) {
        (void)bt_manager_disconnect(g_peer_addr);
        BT_LOGI("disconnect %s", g_peer_addr);
    }

    bt_manager_set_scan_mode(BTMG_SCAN_MODE_NONE);
    g_running = 0;
    BT_LOGI("audio stop: hidden");
    return 0;
}

void app_bt_audio_deinit(void)
{
    if (!g_cb) return;

    app_bt_audio_stop();

    g_shutting_down = 1;
    disable_stream_callbacks();
    clear_all_callbacks();
    usleep(200 * 1000);

    bt_manager_enable(false);
    usleep(300 * 1000);

    bt_manager_deinit(g_cb);
    g_cb = NULL;
    btmg_log_stop();

    g_shutting_down = 0;
}

void app_bt_audio_set_discoverable(bool on)
{
    bt_manager_set_scan_mode(on ? BTMG_SCAN_MODE_CONNECTABLE_DISCOVERABLE
                                : BTMG_SCAN_MODE_NONE);
}

#endif