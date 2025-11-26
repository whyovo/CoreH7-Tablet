/**
 * @file lv_port_disp_templ.c
 *
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_disp.h"
#include <stdbool.h>
#include "lcd_rgb.h"

extern LTDC_HandleTypeDef hltdc;

/*********************
 *      DEFINES
 *********************/
// #define LVGL_MemoryAdd (RGB_LCD_MemoryAdd + RGB_LCD_Width * RGB_LCD_Height *
// RGB_BytesPerPixel_0) // 显示缓冲区地址
#define LVGL_BufSize (RGB_LCD_Width * RGB_LCD_Height * RGB_BytesPerPixel_0)
#define LVGL_Buf1 (RGB_LCD_MemoryAdd)                // 第一缓冲区
#define LVGL_Buf2 (RGB_LCD_MemoryAdd + LVGL_BufSize) // 第二缓冲区

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);
static void LV_ATTRIBUTE_FAST_MEM disp_flush(lv_disp_drv_t *disp_drv,
                                            const lv_area_t *area,
                                            lv_color_t *color_p);
//static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
//        const lv_area_t * fill_area, lv_color_t color);

/**********************
 *  STATIC VARIABLES
 **********************/
//  FPS 计数变量
static volatile uint32_t real_fps_counter = 0;
static volatile uint32_t real_fps_display = 0;
static volatile uint32_t fps_last_time = 0;
/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_disp_init(void)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();

    /*-----------------------------
     * Create a buffer for drawing
     *----------------------------*/

    /**
     * LVGL requires a buffer where it internally draws the widgets.
     * Later this buffer will passed to your display driver's `flush_cb` to copy its content to your display.
     * The buffer has to be greater than 1 display row
     *
     * There are 3 buffering configurations:
     * 1. Create ONE buffer:
     *      LVGL will draw the display's content here and writes it to your display
     *
     * 2. Create TWO buffer:
     *      LVGL will draw the display's content to a buffer and writes it your display.
     *      You should use DMA to write the buffer's content to the display.
     *      It will enable LVGL to draw the next part of the screen to the other buffer while
     *      the data is being sent form the first buffer. It makes rendering and flushing parallel.
     *
     * 3. Double buffering
     *      Set 2 screens sized buffers and set disp_drv.full_refresh = 1.
     *      This way LVGL will always provide the whole rendered screen in `flush_cb`
     *      and you only need to change the frame buffer's address.
     */

    /* Example for 1) */
//    static lv_disp_draw_buf_t draw_buf_dsc_1;
//    static lv_color_t buf_1[MY_DISP_HOR_RES * 10];                          /*A buffer for 10 rows*/
//    lv_disp_draw_buf_init(&draw_buf_dsc_1, buf_1, NULL, MY_DISP_HOR_RES * 10);   /*Initialize the display buffer*/

//    /* Example for 2) */
//    static lv_disp_draw_buf_t draw_buf_dsc_2;
//    static lv_color_t buf_2_1[MY_DISP_HOR_RES * 10];                        /*A buffer for 10 rows*/
//    static lv_color_t buf_2_2[MY_DISP_HOR_RES * 10];                        /*An other buffer for 10 rows*/
//    lv_disp_draw_buf_init(&draw_buf_dsc_2, buf_2_1, buf_2_2, MY_DISP_HOR_RES * 10);   /*Initialize the display buffer*/

    /* Example for 3) also set disp_drv.full_refresh = 1 below*/
    static lv_disp_draw_buf_t draw_buf_dsc_3;
    // static lv_color_t *buf_3_1 = (lv_color_t * )(LVGL_MemoryAdd); /*A screen
    // sized buffer*/ static lv_color_t *buf_3_2 = (lv_color_t *
    // )(LVGL_MemoryAdd + RGB_LCD_Width*RGB_LCD_Height*RGB_BytesPerPixel_0);
    static lv_color_t *buf_3_1 = (lv_color_t *)(LVGL_Buf1); /*第一屏缓冲区*/
    static lv_color_t *buf_3_2 = (lv_color_t *)(LVGL_Buf2); /*第二屏缓冲区*/
    lv_disp_draw_buf_init(&draw_buf_dsc_3, buf_3_1, buf_3_2,
                          RGB_LCD_Width * RGB_LCD_Height);   /*Initialize the display buffer*/

    /*-----------------------------------
     * Register the display in LVGL
     *----------------------------------*/

    static lv_disp_drv_t disp_drv;                         /*Descriptor of a display driver*/
    lv_disp_drv_init(&disp_drv);                    /*Basic initialization*/

    /*Set up the functions to access to your display*/

    /*Set the resolution of the display*/
    disp_drv.hor_res = RGB_LCD_Width;
    disp_drv.ver_res = RGB_LCD_Height;

    /*Used to copy the buffer's content to the display*/
    disp_drv.flush_cb = disp_flush;

    /*Set a display buffer*/
    disp_drv.draw_buf = &draw_buf_dsc_3;

    /*Required for Example 3)*/
    disp_drv.full_refresh = 1;

    /* Fill a memory array with a color if you have GPU.
     * Note that, in lv_conf.h you can enable GPUs that has built-in support in LVGL.
     * But if you have a different GPU you can use with this callback.*/
    //disp_drv.gpu_fill_cb = gpu_fill;

    /*Finally register the driver*/
    lv_disp_drv_register(&disp_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*Initialize your display and the required peripherals.*/
static void disp_init(void) {
  /* 配置LTDC Layer1显示缓冲区起始地址 */
  // LTDC_Layer1->CFBAR = (uint32_t)(LVGL_Buf1);

  /* 使能LTDC Line Event中断 */
  __HAL_LTDC_ENABLE_IT(&hltdc, LTDC_IT_LI);

  /* 编程Line Event */
  HAL_LTDC_ProgramLineEvent(&hltdc, 0);
  /*You code here*/
}

volatile bool disp_flush_enabled = true;

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

// 添加一个指针用于在中断中通知LVGL
static lv_disp_drv_t *disp_drv_flush_pending = NULL;

/**
  * @brief  Line Event callback.
  * @param  hltdc: pointer to a LTDC_HandleTypeDef structure that contains
  *                the configuration information for the specified LTDC.
  * @retval None
  */
// void LV_ATTRIBUTE_FAST_MEM HAL_LTDC_LineEvenCallback(LTDC_HandleTypeDef *hltdc) {
//   // 重新载入参数，新显存地址生效，此时显示才会更新
//   // 每次进入中断才会更新显示，这样能有效避免撕裂现象
//   __HAL_LTDC_RELOAD_CONFIG(hltdc);
//   HAL_LTDC_ProgramLineEvent(hltdc, 0);
//   //	LED1_Toggle;
// }
void LV_ATTRIBUTE_FAST_MEM
HAL_LTDC_LineEvenCallback(LTDC_HandleTypeDef *hltdc) {
  /*
     当进入此中断时（Line 0），意味着上一帧的扫描已经结束，
     且如果在 disp_flush 中请求了 RELOAD，新的 CFBAR 地址此时已经生效。
     现在显示器正在显示新的一帧，我们可以安全地告诉 LVGL 去画下一帧了。
  */
  // ===== 真实 FPS 计数开始 =====
  uint32_t current_time = lv_tick_get();
  real_fps_counter++;

  if (current_time - fps_last_time >= 1000) { // 每 1000ms 更新一次
    real_fps_display = real_fps_counter;
    real_fps_counter = 0;
    fps_last_time = current_time;
  }
  // ===== 真实 FPS 计数结束 =====
  if (disp_drv_flush_pending != NULL) {
    lv_disp_flush_ready(disp_drv_flush_pending);
    disp_drv_flush_pending = NULL;
  }

  // 重新编程 Line Event 以便下一帧触发中断
  HAL_LTDC_ProgramLineEvent(hltdc, 0);
}
// 提供获取实时 FPS 的接口函数
uint32_t lv_port_get_real_fps(void) { return real_fps_display; }
/*Flush the content of the internal buffer the specific area on the display
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_disp_flush_ready()' has to be called when finished.*/
// static void disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
// {
//     if(disp_flush_enabled) {
//       SCB_CleanDCache_by_Addr((uint32_t *)color_p, LVGL_BufSize);

//       LTDC_Layer1->CFBAR = (uint32_t)color_p; // 切换显存地址
//     }

//     // DEBUG_INFO("flush!");

//     /*IMPORTANT!!!
//      *Inform the graphics library that you are ready with the flushing*/
//     lv_disp_flush_ready(disp_drv);
// }
static void disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area,
                       lv_color_t *color_p) {
  if (disp_flush_enabled) {
    /* 清理 D-Cache，确保数据从 Cache 写入 RAM，否则屏幕会显示花屏或旧数据 、
不用了！dcache别开缓存，开缓冲就行*/
    // SCB_CleanDCache_by_Addr((uint32_t *)color_p, LVGL_BufSize);

    /*  切换 LTDC 显存地址 */
    LTDC_Layer1->CFBAR = (uint32_t)color_p;

    /* 请求 LTDC 在下一个 VSYNC 重载配置 (Vertical Blanking Reload) */
    __HAL_LTDC_RELOAD_CONFIG(&hltdc);

    /*  保存句柄，等待中断通知 LVGL */
    disp_drv_flush_pending = disp_drv;
  } else {
    /* 如果刷新被禁用，直接通知完成以避免死锁 */
    lv_disp_flush_ready(disp_drv);
  }

  /* 注意：这里不再调用 lv_disp_flush_ready(disp_drv); */
  /* 它被移动到了 HAL_LTDC_LineEvenCallback 中 */
}
/*OPTIONAL: GPU INTERFACE*/

/*If your MCU has hardware accelerator (GPU) then you can use it to fill a memory with a color*/
//static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
//                    const lv_area_t * fill_area, lv_color_t color)
//{
//    /*It's an example code which should be done by your GPU*/
//    int32_t x, y;
//    dest_buf += dest_width * fill_area->y1; /*Go to the first line*/
//
//    for(y = fill_area->y1; y <= fill_area->y2; y++) {
//        for(x = fill_area->x1; x <= fill_area->x2; x++) {
//            dest_buf[x] = color;
//        }
//        dest_buf+=dest_width;    /*Go to the next line*/
//    }
//}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
