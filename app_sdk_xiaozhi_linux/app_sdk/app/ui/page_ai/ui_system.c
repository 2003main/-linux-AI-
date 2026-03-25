
#include "cfg.h"
#include "ipc_udp.h"
#include "cJSON/cJSON.h"
#include "page_ai.h"
#include "lang_config.h"
#include "image_conf.h"

// 定义设备状态枚举类型
typedef enum DeviceState {
    kDeviceStateUnknown,
    kDeviceStateStarting,
    kDeviceStateWifiConfiguring,
    kDeviceStateIdle,
    kDeviceStateConnecting,
    kDeviceStateListening,
    kDeviceStateSpeaking,
    kDeviceStateUpgrading,
    kDeviceStateActivating,
    kDeviceStateFatalError
} DeviceState;

// 静态变量，用于存储IPC端点
static p_ipc_endpoint_t g_ipc_ep;

// 将设备状态转换为本地字符串
static const char* ConvertToLocalString(DeviceState state)
{
    switch (state) {
        case kDeviceStateUnknown:
            return UNKNOWN_STATUS;
        case kDeviceStateStarting:
            return INITIALIZING;
        case kDeviceStateWifiConfiguring:
            return NETWORK_CFG;
        case kDeviceStateIdle:
            return STANDBY;
        case kDeviceStateConnecting:
            return CONNECTING;
        case kDeviceStateListening:
            return LISTENING;
        case kDeviceStateSpeaking:
            return SPEAKING;
        case kDeviceStateUpgrading:
            return UPGRADING;
        case kDeviceStateActivating:
            return ACTIVATION;
        case kDeviceStateFatalError:
            return ERROR_STR;
    }

    return "未知状态";
}

/*
 * 处理从IPC接收到的UI数据。
 * 处理的数据格式:
 * 1. 状态: {"state": 0}等, 取值对应DeviceState的取值
 * 2. 要显示的文本: {"text": "你好"}
 * 3. 要显示的emotion: {"emotion": "happy"}, 有这些取值:
 *           "neutral","happy","laughing","funny","sad","angry","crying","loving",
 *           "embarrassed","surprised","shocked","thinking","winking","cool","relaxed",
 *           "delicious","kissy","confident","sleepy","silly","confused"
 * 4. WIFI强度: {"wifi": "100"}
 * 5. 电量: {"battery": "100"}
 *
 * @param buffer 包含JSON格式数据的字符串缓冲区
 * @param size 缓冲区的大小
 * @param user_data 用户数据指针（未使用）
 * @return 0 表示成功，-1 表示解析错误
 */
static int process_ui_data(char *buffer, size_t size, void *user_data)
{
    cJSON *json;

    // 解析JSON数据
    json = cJSON_Parse(buffer);
    if (!json) {
        printf("cJSON_Parse err: %s ", buffer);
        return -1;
    }

    // 获取状态字段
    cJSON *state = cJSON_GetObjectItem(json, "state");
    if (state) {
        printf("get state = %d, %s \n", state->valueint, ConvertToLocalString((DeviceState)state->valueint));
        SetStateString(ConvertToLocalString((DeviceState)state->valueint));
    }

    //获取情绪
    cJSON *emotion = cJSON_GetObjectItem(json,"emotion");
    if (emotion && emotion->type == cJSON_String) {
        const char *emotion_str = emotion->valuestring;
        printf("获取到情绪: %s\n", emotion_str);
        const char *image_name = NULL; // 用于存储最终确定的图片名

        // 将情绪字符串映射到具体的图片文件名
        if (strcmp(emotion_str, "happy") == 0 || strcmp(emotion_str, "laughing") == 0) {
            image_name = "img_happy.png";
        } else if (strcmp(emotion_str, "neutral") == 0 || strcmp(emotion_str, "thinking") == 0 || strcmp(emotion_str, "relaxed") == 0) {
            image_name = "img_neutral.png";
        } else if (strcmp(emotion_str, "sad") == 0 || strcmp(emotion_str, "crying") == 0) {
            image_name = "img_sad.png";
        } else if (strcmp(emotion_str, "angry") == 0) {
            image_name = "img_angry.png";
        } else if (strcmp(emotion_str, "cool") == 0 || strcmp(emotion_str, "confident") == 0) {
            image_name = "img_happy.png";
        } else if (strcmp(emotion_str, "surprised") == 0 || strcmp(emotion_str, "shocked") == 0) {
            image_name = "img_surprised.png";
        } else if (strcmp(emotion_str, "kissy") == 0 || strcmp(emotion_str, "loving") == 0) {
            image_name = "img_happy.png";
        } else if (strcmp(emotion_str, "winking") == 0 || strcmp(emotion_str, "funny") == 0 || strcmp(emotion_str, "silly") == 0) {
            image_name = "img_angry.png";
        } else if (strcmp(emotion_str, "delicious") == 0) {
            image_name = "img_happy.png";
        } else if (strcmp(emotion_str, "sleepy") == 0) {
            image_name = "img_sad.png";
        } else if (strcmp(emotion_str, "embarrassed") == 0) {
            image_name = "img_embarrassed.png";
        } else if (strcmp(emotion_str, "confused") == 0) {
            image_name = "img_neutral.png";
        } else {
            // 未识别的情绪，使用默认或中性表情
            image_name = "img_neutral.png";
            printf("未识别的情绪类型，使用默认表情。\n");
        }

        // 构建完整的图片路径并设置
        char image_path[256];
        snprintf(image_path, sizeof(image_path), GET_IMAGE_PATH("%s"), image_name);
        SetEmotion(image_path);
    }

    // 获取文本字段
    cJSON *text = cJSON_GetObjectItem(json, "text");
    if (text) {
        printf("get text = %s ", text->valuestring);
        SetText(text->valuestring);
    }

    // 获取WIFI强度字段（未处理）
    cJSON *wifi = cJSON_GetObjectItem(json, "wifi");
    if (wifi) {
        // 处理WIFI强度
    }

    // 获取电量字段（未处理）
    cJSON *battery = cJSON_GetObjectItem(json, "battery");
    if (battery) {
        // 处理电量
    }

    // 释放JSON对象
    cJSON_Delete(json);

    return 0;
}

/*
 * 初始化UI系统。
 * 创建IPC端点，用于接收和处理UI数据。
 *
 * @return 0 表示成功，-1 表示创建IPC端点失败
 */
int ui_system_init(void)
{
    // 创建UDP IPC端点
    printf("ui_system_init\n");
    g_ipc_ep = ipc_endpoint_create_udp(UI_PORT_DOWN, UI_PORT_UP, process_ui_data, NULL);
    if (!g_ipc_ep) {
        printf("Failed to create IPC endpoint\n");
        return -1;
    }
    return 0;
}