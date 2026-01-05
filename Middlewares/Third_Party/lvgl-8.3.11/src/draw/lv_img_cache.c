/**
 * @file lv_img_cache.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../misc/lv_assert.h"
#include "lv_img_cache.h"
#include "lv_img_decoder.h"
#include "lv_draw_img.h"
#include "../hal/lv_hal_tick.h"
#include "../misc/lv_gc.h"

/*********************
 *      DEFINES
 *********************/
/*Decrement life with this value on every open*/
#define LV_IMG_CACHE_AGING 1

/*Boost life by this factor (multiply time_to_open with this value)*/
#define LV_IMG_CACHE_LIFE_GAIN 1

/*Don't let life to be greater than this limit because it would require a lot of time to
 * "die" from very high values*/
#define LV_IMG_CACHE_LIFE_LIMIT 1000

/* 自定义背景图限制配置 */
#define BG_IMG_WIDTH 800
#define BG_IMG_HEIGHT 480
#define BG_IMG_CACHE_LIMIT 3

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
#if LV_IMG_CACHE_DEF_SIZE
static bool lv_img_cache_match(const void *src1, const void *src2);
#endif

/**********************
 *  STATIC VARIABLES
 **********************/
#if LV_IMG_CACHE_DEF_SIZE
static uint16_t entry_cnt;
#endif

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**
 * Open an image using the image decoder interface and cache it.
 * The image will be left open meaning if the image decoder open callback allocated memory then it will remain.
 * The image is closed if a new image is opened and the new image takes its place in the cache.
 * @param src source of the image. Path to file or pointer to an `lv_img_dsc_t` variable
 * @param color color The color of the image with `LV_IMG_CF_ALPHA_...`
 * @return pointer to the cache entry or NULL if can open the image
 */
_lv_img_cache_entry_t *_lv_img_cache_open(const void *src, lv_color_t color, int32_t frame_id)
{
    /*Is the image cached?*/
    _lv_img_cache_entry_t *cached_src = NULL;

#if LV_IMG_CACHE_DEF_SIZE
    if (entry_cnt == 0)
    {
        LV_LOG_WARN("lv_img_cache_open: the cache size is 0");
        return NULL;
    }

    _lv_img_cache_entry_t *cache = LV_GC_ROOT(_lv_img_cache_array);

    /*Decrement all lifes. Make the entries older*/
    uint16_t i;
    for (i = 0; i < entry_cnt; i++)
    {
        if (cache[i].life > INT32_MIN + LV_IMG_CACHE_AGING)
        {
            cache[i].life -= LV_IMG_CACHE_AGING;
        }
    }

    for (i = 0; i < entry_cnt; i++)
    {
        if (color.full == cache[i].dec_dsc.color.full &&
            frame_id == cache[i].dec_dsc.frame_id &&
            lv_img_cache_match(src, cache[i].dec_dsc.src))
        {
            /*If opened increment its life.
             *Image difficult to open should live longer to keep avoid frequent their recaching.
             *Therefore increase `life` with `time_to_open`*/
            cached_src = &cache[i];
            cached_src->life += cached_src->dec_dsc.time_to_open * LV_IMG_CACHE_LIFE_GAIN;
            if (cached_src->life > LV_IMG_CACHE_LIFE_LIMIT)
                cached_src->life = LV_IMG_CACHE_LIFE_LIMIT;
            LV_LOG_TRACE("image source found in the cache");
            break;
        }
    }

    /*The image is not cached then cache it now*/
    if (cached_src)
        return cached_src;

    /*Find an entry to reuse. Select the entry with the least life*/
    /* 针对 800x480 背景图的特殊缓存策略 */

    // 1. 预先获取请求图片的尺寸信息
    lv_img_header_t header;
    bool is_bg_req = false;
    if (lv_img_decoder_get_info(src, &header) == LV_RES_OK)
    {
        if (header.w == BG_IMG_WIDTH && header.h == BG_IMG_HEIGHT)
        {
            is_bg_req = true;
        }
    }

    // 2. 统计当前缓存中的背景图数量，并找到最弱的背景图和最弱的任意图
    int bg_cnt = 0;
    _lv_img_cache_entry_t *weakest_bg = NULL;
    _lv_img_cache_entry_t *weakest_any = &cache[0];

    for (i = 0; i < entry_cnt; i++)
    {
        // 寻找全局最弱（寿命最短）的项
        if (cache[i].life < weakest_any->life)
        {
            weakest_any = &cache[i];
        }

        // 检查该缓存项是否是背景图 (前提是该项已被占用)
        if (cache[i].dec_dsc.src != NULL &&
            cache[i].dec_dsc.header.w == BG_IMG_WIDTH &&
            cache[i].dec_dsc.header.h == BG_IMG_HEIGHT)
        {

            bg_cnt++;
            // 寻找背景图中最弱的一项
            if (weakest_bg == NULL || cache[i].life < weakest_bg->life)
            {
                weakest_bg = &cache[i];
            }
        }
    }

    // 3. 决策：选择哪个槽位进行覆盖
    if (is_bg_req && bg_cnt >= BG_IMG_CACHE_LIMIT)
    {
        // 如果请求的是背景图，且缓存已满3张，必须踢掉一张旧的背景图
        if (weakest_bg != NULL)
        {
            cached_src = weakest_bg;
            LV_LOG_INFO("image draw: bg limit reached, evicting old bg");
        }
        else
        {
            // 理论上不应该进这里（bg_cnt > 0 必有 weakest_bg），作为兜底
            cached_src = weakest_any;
        }
    }
    else
    {
        // 其他情况（小图，或者背景图还没存满3张），使用全局最弱淘汰策略
        cached_src = weakest_any;
    }
    /* 修改结束 */

    /*Close the decoder to reuse if it was opened (has a valid source)*/
    if (cached_src->dec_dsc.src)
    {
        lv_img_decoder_close(&cached_src->dec_dsc);
        LV_LOG_INFO("image draw: cache miss, close and reuse an entry");
    }
    else
    {
        LV_LOG_INFO("image draw: cache miss, cached to an empty entry");
    }
#else
    cached_src = &LV_GC_ROOT(_lv_img_cache_single);
#endif
    /*Open the image and measure the time to open*/
    uint32_t t_start = lv_tick_get();
    lv_res_t open_res = lv_img_decoder_open(&cached_src->dec_dsc, src, color, frame_id);
    if (open_res == LV_RES_INV)
    {
        LV_LOG_WARN("Image draw cannot open the image resource");
        lv_memset_00(cached_src, sizeof(_lv_img_cache_entry_t));
        cached_src->life = INT32_MIN; /*Make the empty entry very "weak" to force its us*/
        return NULL;
    }

    cached_src->life = 0;

    /*If `time_to_open` was not set in the open function set it here*/
    if (cached_src->dec_dsc.time_to_open == 0)
    {
        cached_src->dec_dsc.time_to_open = lv_tick_elaps(t_start);
    }

    if (cached_src->dec_dsc.time_to_open == 0)
        cached_src->dec_dsc.time_to_open = 1;

    return cached_src;
}

/**
 * Set the number of images to be cached.
 * More cached images mean more opened image at same time which might mean more memory usage.
 * E.g. if 20 PNG or JPG images are open in the RAM they consume memory while opened in the cache.
 * @param new_entry_cnt number of image to cache
 */
void lv_img_cache_set_size(uint16_t new_entry_cnt)
{
#if LV_IMG_CACHE_DEF_SIZE == 0
    LV_UNUSED(new_entry_cnt);
    LV_LOG_WARN("Can't change cache size because it's disabled by LV_IMG_CACHE_DEF_SIZE = 0");
#else
    if (LV_GC_ROOT(_lv_img_cache_array) != NULL)
    {
        /*Clean the cache before free it*/
        lv_img_cache_invalidate_src(NULL);
        lv_mem_free(LV_GC_ROOT(_lv_img_cache_array));
    }

    /*Reallocate the cache*/
    LV_GC_ROOT(_lv_img_cache_array) = lv_mem_alloc(sizeof(_lv_img_cache_entry_t) * new_entry_cnt);
    LV_ASSERT_MALLOC(LV_GC_ROOT(_lv_img_cache_array));
    if (LV_GC_ROOT(_lv_img_cache_array) == NULL)
    {
        entry_cnt = 0;
        return;
    }
    entry_cnt = new_entry_cnt;

    /*Clean the cache*/
    lv_memset_00(LV_GC_ROOT(_lv_img_cache_array), entry_cnt * sizeof(_lv_img_cache_entry_t));
#endif
}

/**
 * Invalidate an image source in the cache.
 * Useful if the image source is updated therefore it needs to be cached again.
 * @param src an image source path to a file or pointer to an `lv_img_dsc_t` variable.
 */
void lv_img_cache_invalidate_src(const void *src)
{
    LV_UNUSED(src);
#if LV_IMG_CACHE_DEF_SIZE
    _lv_img_cache_entry_t *cache = LV_GC_ROOT(_lv_img_cache_array);

    uint16_t i;
    for (i = 0; i < entry_cnt; i++)
    {
        if (src == NULL || lv_img_cache_match(src, cache[i].dec_dsc.src))
        {
            if (cache[i].dec_dsc.src != NULL)
            {
                lv_img_decoder_close(&cache[i].dec_dsc);
            }

            lv_memset_00(&cache[i], sizeof(_lv_img_cache_entry_t));
        }
    }
#endif
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

#if LV_IMG_CACHE_DEF_SIZE
static bool lv_img_cache_match(const void *src1, const void *src2)
{
    lv_img_src_t src_type = lv_img_src_get_type(src1);
    if (src_type == LV_IMG_SRC_VARIABLE)
        return src1 == src2;
    if (src_type != LV_IMG_SRC_FILE)
        return false;
    if (lv_img_src_get_type(src2) != LV_IMG_SRC_FILE)
        return false;
    return strcmp(src1, src2) == 0;
}
#endif
