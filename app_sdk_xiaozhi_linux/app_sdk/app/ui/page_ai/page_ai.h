

#ifndef _PAGE_MAIN_H
#define _PAGE_MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif
    void init_page_main(void);

    void SetStateString(const char *str);

    /* 每次只能显示给定的str */
    void SetText(const char *str);

    /*
     *  注意，这里要与 lv_conf.h 中 LV_FS_STDIO_LETTER 或 LV_FS_POSIX_LETTER 指定的盘符一致
     *  示例(Windows)： 假设 LETTER 设置为 'D'，之后路径可 "D:/100ask/img_naughty.png"
     *  示例(Linux)：   直接设置 LETTER 设置为 'A'，之后路径可为 "A:/mnt/img_naughty.png"
     */
    void SetEmotion(const char *jpgFile);

    void OnClicked(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*LV_100ASK_XZ_AI_MAIN_H*/
