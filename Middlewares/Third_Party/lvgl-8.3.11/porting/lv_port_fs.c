/**
 * @file lv_port_fs.c
 *
 */

/*Copy this file as "lv_port_fs.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_fs.h"
#include "lvgl.h"
#include "fatfs.h"
#include <stdio.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void fs_init(void);

static void *fs_open(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode);
static lv_fs_res_t fs_close(lv_fs_drv_t *drv, void *file_p);
static lv_fs_res_t fs_read(lv_fs_drv_t *drv, void *file_p, void *buf, uint32_t btr, uint32_t *br);
static lv_fs_res_t fs_write(lv_fs_drv_t *drv, void *file_p, const void *buf, uint32_t btw, uint32_t *bw);
static lv_fs_res_t fs_seek(lv_fs_drv_t *drv, void *file_p, uint32_t pos, lv_fs_whence_t whence);
static lv_fs_res_t fs_tell(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p);

static void *fs_dir_open(lv_fs_drv_t *drv, const char *path);
static lv_fs_res_t fs_dir_read(lv_fs_drv_t *drv, void *rddir_p, char *fn);
static lv_fs_res_t fs_dir_close(lv_fs_drv_t *drv, void *rddir_p);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_fs_init(void)
{
    /*----------------------------------------------------
     * Initialize your storage device and File System
     * -------------------------------------------------*/
    fs_init();

    /*---------------------------------------------------
     * Register the file system interface in LVGL
     *--------------------------------------------------*/

    /*Add a simple drive to open images*/
    static lv_fs_drv_t fs_drv;
    lv_fs_drv_init(&fs_drv);

    /*Set up fields...*/
    fs_drv.letter = 'S'; /* 'S' for SD Card, usage: "S:folder/file.txt" */
    fs_drv.open_cb = fs_open;
    fs_drv.close_cb = fs_close;
    fs_drv.read_cb = fs_read;
    fs_drv.write_cb = fs_write;
    fs_drv.seek_cb = fs_seek;
    fs_drv.tell_cb = fs_tell;

    fs_drv.dir_close_cb = fs_dir_close;
    fs_drv.dir_open_cb = fs_dir_open;
    fs_drv.dir_read_cb = fs_dir_read;

    lv_fs_drv_register(&fs_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*Initialize your Storage device and File system.*/
static void fs_init(void)
{

}

/**
 * Open a file
 * @param drv       pointer to a driver where this function belongs
 * @param path      path to the file beginning with the driver letter (e.g. S:/folder/file.txt)
 * @param mode      read: FS_MODE_RD, write: FS_MODE_WR, both: FS_MODE_RD | FS_MODE_WR
 * @return          a file descriptor or NULL on error
 */
static void *fs_open(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode)
{
    FIL *fp = lv_mem_alloc(sizeof(FIL));
    if (fp == NULL)
        return NULL;

    BYTE flags = 0;
    if (mode == LV_FS_MODE_WR)
        flags = FA_WRITE | FA_OPEN_ALWAYS;
    else if (mode == LV_FS_MODE_RD)
        flags = FA_READ;
    else if (mode == (LV_FS_MODE_WR | LV_FS_MODE_RD))
        flags = FA_READ | FA_WRITE | FA_OPEN_ALWAYS;

    char full_path[256];
    /* FatFS expects "0:/path" */
    if (path[0] == '/')
    {
        snprintf(full_path, sizeof(full_path), "0:%s", path);
    }
    else
    {
        snprintf(full_path, sizeof(full_path), "0:/%s", path);
    }

    FRESULT res = f_open(fp, full_path, flags);
    if (res != FR_OK)
    {
        lv_mem_free(fp);
        return NULL;
    }
    return fp;
}

/**
 * Close an opened file
 * @param drv       pointer to a driver where this function belongs
 * @param file_p    pointer to a file_t variable. (opened with fs_open)
 * @return          LV_FS_RES_OK: no error or  any error from @lv_fs_res_t enum
 */
static lv_fs_res_t fs_close(lv_fs_drv_t *drv, void *file_p)
{
    f_close((FIL *)file_p);
    lv_mem_free(file_p);
    return LV_FS_RES_OK;
}

/**
 * Read data from an opened file
 * @param drv       pointer to a driver where this function belongs
 * @param file_p    pointer to a file_t variable.
 * @param buf       pointer to a memory block where to store the read data
 * @param btr       number of Bytes To Read
 * @param br        the real number of read bytes (Byte Read)
 * @return          LV_FS_RES_OK: no error or  any error from @lv_fs_res_t enum
 */
static lv_fs_res_t fs_read(lv_fs_drv_t *drv, void *file_p, void *buf, uint32_t btr, uint32_t *br)
{
    FRESULT res = f_read((FIL *)file_p, buf, btr, (UINT *)br);
    if (res == FR_OK)
        return LV_FS_RES_OK;
    return LV_FS_RES_UNKNOWN;
}

/**
 * Write into a file
 * @param drv       pointer to a driver where this function belongs
 * @param file_p    pointer to a file_t variable
 * @param buf       pointer to a buffer with the bytes to write
 * @param btw       Bytes To Write
 * @param bw        the number of real written bytes (Bytes Written). NULL if unused.
 * @return          LV_FS_RES_OK: no error or  any error from @lv_fs_res_t enum
 */
static lv_fs_res_t fs_write(lv_fs_drv_t *drv, void *file_p, const void *buf, uint32_t btw, uint32_t *bw)
{
    FRESULT res = f_write((FIL *)file_p, buf, btw, (UINT *)bw);
    if (res == FR_OK)
        return LV_FS_RES_OK;
    return LV_FS_RES_UNKNOWN;
}

/**
 * Set the read write pointer. Also expand the file size if necessary.
 * @param drv       pointer to a driver where this function belongs
 * @param file_p    pointer to a file_t variable. (opened with fs_open )
 * @param pos       the new position of read write pointer
 * @param whence    tells from where to interpret the `pos`. See @lv_fs_whence_t
 * @return          LV_FS_RES_OK: no error or  any error from @lv_fs_res_t enum
 */
static lv_fs_res_t fs_seek(lv_fs_drv_t *drv, void *file_p, uint32_t pos, lv_fs_whence_t whence)
{
    FIL *fp = (FIL *)file_p;
    uint32_t new_pos = 0;
    switch (whence)
    {
    case LV_FS_SEEK_SET:
        new_pos = pos;
        break;
    case LV_FS_SEEK_CUR:
        new_pos = f_tell(fp) + pos;
        break;
    case LV_FS_SEEK_END:
        new_pos = f_size(fp) + pos;
        break;
    }
    FRESULT res = f_lseek(fp, new_pos);
    if (res == FR_OK)
        return LV_FS_RES_OK;
    return LV_FS_RES_UNKNOWN;
}
/**
 * Give the position of the read write pointer
 * @param drv       pointer to a driver where this function belongs
 * @param file_p    pointer to a file_t variable.
 * @param pos_p     pointer to to store the result
 * @return          LV_FS_RES_OK: no error or  any error from @lv_fs_res_t enum
 */
static lv_fs_res_t fs_tell(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p)
{
    *pos_p = f_tell((FIL *)file_p);
    return LV_FS_RES_OK;
}

/**
 * Initialize a 'lv_fs_dir_t' variable for directory reading
 * @param drv       pointer to a driver where this function belongs
 * @param path      path to a directory
 * @return          pointer to the directory read descriptor or NULL on error
 */
static void *fs_dir_open(lv_fs_drv_t *drv, const char *path)
{
    DIR *dir = lv_mem_alloc(sizeof(DIR));
    if (dir == NULL)
        return NULL;

    char full_path[256];
    if (path[0] == '/')
    {
        snprintf(full_path, sizeof(full_path), "0:%s", path);
    }
    else
    {
        snprintf(full_path, sizeof(full_path), "0:/%s", path);
    }

    FRESULT res = f_opendir(dir, full_path);
    if (res != FR_OK)
    {
        lv_mem_free(dir);
        return NULL;
    }
    return dir;
}

/**
 * Read the next filename form a directory.
 * The name of the directories will begin with '/'
 * @param drv       pointer to a driver where this function belongs
 * @param rddir_p   pointer to an initialized 'lv_fs_dir_t' variable
 * @param fn        pointer to a buffer to store the filename
 * @return          LV_FS_RES_OK: no error or  any error from @lv_fs_res_t enum
 */
static lv_fs_res_t fs_dir_read(lv_fs_drv_t *drv, void *rddir_p, char *fn)
{
    FILINFO fno;
    FRESULT res = f_readdir((DIR *)rddir_p, &fno);
    if (res != FR_OK || fno.fname[0] == 0)
        return LV_FS_RES_UNKNOWN;

    if (fno.fattrib & AM_DIR)
    {
        fn[0] = '/';
        strncpy(&fn[1], fno.fname, 255);
    }
    else
    {
        strncpy(fn, fno.fname, 256);
    }
    return LV_FS_RES_OK;
}

/**
 * Close the directory reading
 * @param drv       pointer to a driver where this function belongs
 * @param rddir_p   pointer to an initialized 'lv_fs_dir_t' variable
 * @return          LV_FS_RES_OK: no error or  any error from @lv_fs_res_t enum
 */
static lv_fs_res_t fs_dir_close(lv_fs_drv_t *drv, void *rddir_p)
{
    f_closedir((DIR *)rddir_p);
    lv_mem_free(rddir_p);
    return LV_FS_RES_OK;
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
