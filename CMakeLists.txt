cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME 4k_music_game)

# Include toolchain file
include("cmake/gcc-arm-none-eabi.cmake")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# 禁用 RELRO 以解决链接器内存布局错误
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE -Wl,-z,norelro)

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    ${CMAKE_SOURCE_DIR}/BSP/init.c
    ${CMAKE_SOURCE_DIR}/BSP/user_hal_callbacks.c

    ${CMAKE_SOURCE_DIR}/BSP/DEBUG/debug.c

    ${CMAKE_SOURCE_DIR}/BSP/FMC/sdram.c

    ${CMAKE_SOURCE_DIR}/BSP/GPIO/key.c
    ${CMAKE_SOURCE_DIR}/BSP/GPIO/led.c

    ${CMAKE_SOURCE_DIR}/BSP/I2S/dmic.c
    ${CMAKE_SOURCE_DIR}/BSP/I2S/dspeaker.c
    ${CMAKE_SOURCE_DIR}/BSP/I2S/audio_player.c

    ${CMAKE_SOURCE_DIR}/BSP/JPEG/jpeg_app.c
    ${CMAKE_SOURCE_DIR}/BSP/JPEG/jpeg_code.c
    ${CMAKE_SOURCE_DIR}/BSP/JPEG/my_jpeg_utils.c

    ${CMAKE_SOURCE_DIR}/BSP/LTDC/lcd_rgb.c
    ${CMAKE_SOURCE_DIR}/BSP/LTDC/lcd_touch.c
    ${CMAKE_SOURCE_DIR}/BSP/LTDC/touch_iic.c

    ${CMAKE_SOURCE_DIR}/BSP/QSPI/flash_font.c
    ${CMAKE_SOURCE_DIR}/BSP/QSPI/qspi_flash.c

    ${CMAKE_SOURCE_DIR}/BSP/SDIO/fatfs.c
    ${CMAKE_SOURCE_DIR}/BSP/SDIO/sdmmc_sd.c
    ${CMAKE_SOURCE_DIR}/BSP/SDIO/FatFs/diskio.c
    ${CMAKE_SOURCE_DIR}/BSP/SDIO/FatFs/ff_gen_drv.c
    ${CMAKE_SOURCE_DIR}/BSP/SDIO/FatFs/ff.c
    ${CMAKE_SOURCE_DIR}/BSP/SDIO/FatFs/ffunicode.c
    ${CMAKE_SOURCE_DIR}/BSP/SDIO/FatFs/sd_diskio.c

    ${CMAKE_SOURCE_DIR}/BSP/SPI/lcd_fonts.c
    ${CMAKE_SOURCE_DIR}/BSP/SPI/lcd_image.c
    ${CMAKE_SOURCE_DIR}/BSP/SPI/lcd_spi.c

    # USB Device Stack
    ${CMAKE_SOURCE_DIR}/BSP/USB/usb_device.c
    ${CMAKE_SOURCE_DIR}/BSP/USB/usbd_conf.c
    ${CMAKE_SOURCE_DIR}/BSP/USB/usbd_desc.c
    ${CMAKE_SOURCE_DIR}/BSP/USB/usbd_cdc_if.c
    ${CMAKE_SOURCE_DIR}/BSP/USB/usbd_dfu_if.c
    ${CMAKE_SOURCE_DIR}/BSP/USB/usbd_storage_if.c

    # USB Host Stack
    ${CMAKE_SOURCE_DIR}/BSP/USB/usb_host.c
    ${CMAKE_SOURCE_DIR}/BSP/USB/usbh_conf.c

    # STM32 USB Device Library - Core
    ${CMAKE_SOURCE_DIR}/BSP/USB/STM32_USB_Device_Library/Core/Src/usbd_core.c
    ${CMAKE_SOURCE_DIR}/BSP/USB/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c
    ${CMAKE_SOURCE_DIR}/BSP/USB/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c

    # STM32 USB Device Library - CDC Class
    ${CMAKE_SOURCE_DIR}/BSP/USB/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c

    # STM32 USB Device Library - DFU Class
    ${CMAKE_SOURCE_DIR}/BSP/USB/STM32_USB_Device_Library/Class/DFU/Src/usbd_dfu.c

    # STM32 USB Device Library - MSC Class
    ${CMAKE_SOURCE_DIR}/BSP/USB/STM32_USB_Device_Library/Class/MSC/Src/usbd_msc.c
    ${CMAKE_SOURCE_DIR}/BSP/USB/STM32_USB_Device_Library/Class/MSC/Src/usbd_msc_bot.c
    ${CMAKE_SOURCE_DIR}/BSP/USB/STM32_USB_Device_Library/Class/MSC/Src/usbd_msc_data.c
    ${CMAKE_SOURCE_DIR}/BSP/USB/STM32_USB_Device_Library/Class/MSC/Src/usbd_msc_scsi.c

    # STM32 USB Host Library - Core
    ${CMAKE_SOURCE_DIR}/BSP/USB/STM32_USB_Host_Library/Core/Src/usbh_core.c
    ${CMAKE_SOURCE_DIR}/BSP/USB/STM32_USB_Host_Library/Core/Src/usbh_ctlreq.c
    ${CMAKE_SOURCE_DIR}/BSP/USB/STM32_USB_Host_Library/Core/Src/usbh_ioreq.c
    ${CMAKE_SOURCE_DIR}/BSP/USB/STM32_USB_Host_Library/Core/Src/usbh_pipes.c

    # STM32 USB Host Library - HID Class
    ${CMAKE_SOURCE_DIR}/BSP/USB/STM32_USB_Host_Library/Class/HID/Src/usbh_hid.c
    ${CMAKE_SOURCE_DIR}/BSP/USB/STM32_USB_Host_Library/Class/HID/Src/usbh_hid_keybd.c
    ${CMAKE_SOURCE_DIR}/BSP/USB/STM32_USB_Host_Library/Class/HID/Src/usbh_hid_mouse.c
    ${CMAKE_SOURCE_DIR}/BSP/USB/STM32_USB_Host_Library/Class/HID/Src/usbh_hid_parser.c

    # LVGL Source Files - Core
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/core/lv_disp.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/core/lv_event.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/core/lv_group.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/core/lv_indev.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/core/lv_indev_scroll.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/core/lv_obj.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/core/lv_obj_class.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/core/lv_obj_draw.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/core/lv_obj_pos.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/core/lv_obj_scroll.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/core/lv_obj_style.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/core/lv_obj_style_gen.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/core/lv_obj_tree.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/core/lv_refr.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/core/lv_theme.c

    # LVGL Source Files - Draw
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/lv_draw.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/lv_draw_arc.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/lv_draw_img.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/lv_draw_label.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/lv_draw_line.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/lv_draw_mask.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/lv_draw_rect.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/lv_draw_transform.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/lv_draw_triangle.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/lv_img_buf.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/lv_img_cache.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/lv_img_decoder.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/sw/lv_draw_sw.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/sw/lv_draw_sw_arc.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/sw/lv_draw_sw_blend.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/sw/lv_draw_sw_dither.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/sw/lv_draw_sw_gradient.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/sw/lv_draw_sw_img.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/sw/lv_draw_sw_letter.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/sw/lv_draw_sw_line.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/sw/lv_draw_sw_polygon.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/sw/lv_draw_sw_rect.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/stm32_dma2d/lv_gpu_stm32_dma2d.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/lv_draw_layer.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/sw/lv_draw_sw.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/sw/lv_draw_sw_arc.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/sw/lv_draw_sw_blend.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/sw/lv_draw_sw_dither.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/sw/lv_draw_sw_gradient.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/sw/lv_draw_sw_img.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/sw/lv_draw_sw_layer.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/sw/lv_draw_sw_letter.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/sw/lv_draw_sw_line.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/sw/lv_draw_sw_polygon.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/sw/lv_draw_sw_rect.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/sw/lv_draw_sw_transform.c

    # LVGL Source Files - Extra Layouts
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/layouts/flex/lv_flex.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/layouts/grid/lv_grid.c

    # LVGL Source Files - Extra Widgets
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/animimg/lv_animimg.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/calendar/lv_calendar.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/calendar/lv_calendar_header_arrow.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/calendar/lv_calendar_header_dropdown.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/chart/lv_chart.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/colorwheel/lv_colorwheel.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/imgbtn/lv_imgbtn.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/keyboard/lv_keyboard.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/led/lv_led.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/list/lv_list.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/menu/lv_menu.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/meter/lv_meter.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/msgbox/lv_msgbox.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/span/lv_span.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/spinbox/lv_spinbox.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/spinner/lv_spinner.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/tabview/lv_tabview.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/tileview/lv_tileview.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/win/lv_win.c

    # LVGL Source Files - Extra Libs
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/libs/bmp/lv_bmp.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/libs/png/lv_png.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/libs/png/lodepng.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/libs/gif/lv_gif.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/libs/gif/gifdec.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/libs/qrcode/lv_qrcode.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/libs/qrcode/qrcodegen.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/libs/sjpg/lv_sjpg.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/libs/sjpg/tjpgd.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/libs/fsdrv/lv_fs_fatfs.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/libs/fsdrv/lv_fs_littlefs.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/libs/fsdrv/lv_fs_posix.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/libs/fsdrv/lv_fs_stdio.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/libs/fsdrv/lv_fs_win32.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/libs/freetype/lv_freetype.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/libs/ffmpeg/lv_ffmpeg.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/libs/rlottie/lv_rlottie.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/libs/tiny_ttf/lv_tiny_ttf.c

    # LVGL Source Files - Extra Themes
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/themes/default/lv_theme_default.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/themes/basic/lv_theme_basic.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/themes/mono/lv_theme_mono.c

    # LVGL Source Files - Extra Others
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/others/fragment/lv_fragment.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/others/fragment/lv_fragment_manager.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/others/gridnav/lv_gridnav.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/others/imgfont/lv_imgfont.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/others/monkey/lv_monkey.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/others/msg/lv_msg.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/others/snapshot/lv_snapshot.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/lv_extra.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/others/ime/lv_ime_pinyin.c 

    # LVGL Source Files - Font
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/font/lv_font.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/font/lv_font_fmt_txt.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/font/lv_font_loader.c

    # LVGL Source Files - HAL
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/hal/lv_hal_disp.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/hal/lv_hal_indev.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/hal/lv_hal_tick.c

    # LVGL Source Files - Misc
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/misc/lv_anim.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/misc/lv_anim_timeline.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/misc/lv_area.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/misc/lv_async.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/misc/lv_bidi.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/misc/lv_color.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/misc/lv_fs.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/misc/lv_gc.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/misc/lv_ll.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/misc/lv_log.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/misc/lv_lru.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/misc/lv_math.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/misc/lv_mem.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/misc/lv_printf.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/misc/lv_style.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/misc/lv_style_gen.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/misc/lv_timer.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/misc/lv_tlsf.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/misc/lv_txt.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/misc/lv_txt_ap.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/misc/lv_utils.c

    # LVGL Source Files - Widgets
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/widgets/lv_arc.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/widgets/lv_bar.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/widgets/lv_btn.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/widgets/lv_btnmatrix.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/widgets/lv_canvas.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/widgets/lv_checkbox.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/widgets/lv_dropdown.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/widgets/lv_img.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/widgets/lv_label.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/widgets/lv_line.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/widgets/lv_objx_templ.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/widgets/lv_roller.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/widgets/lv_slider.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/widgets/lv_switch.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/widgets/lv_table.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/widgets/lv_textarea.c

    # LVGL Porting
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/porting/lv_port_disp.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/porting/lv_port_indev.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/porting/lv_font_qspi.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/porting/lv_port_fs.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/porting/lv_port_jpeg.c

    # LVGL Demos
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/benchmark/lv_demo_benchmark.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/keypad_encoder/lv_demo_keypad_encoder.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/lv_demo_music.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/lv_demo_music_main.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/lv_demo_music_list.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/stress/lv_demo_stress.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/widgets/lv_demo_widgets.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/widgets/assets/img_clothes.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/widgets/assets/img_demo_widgets_avatar.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/widgets/assets/img_lvgl_logo.c  
    
    # LVGL Benchmark Demo Assets
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/benchmark/assets/img_benchmark_cogwheel_alpha16.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/benchmark/assets/img_benchmark_cogwheel_argb.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/benchmark/assets/img_benchmark_cogwheel_chroma_keyed.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/benchmark/assets/img_benchmark_cogwheel_indexed16.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/benchmark/assets/img_benchmark_cogwheel_rgb.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/benchmark/assets/img_benchmark_cogwheel_rgb565a8.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/benchmark/assets/lv_font_bechmark_montserrat_12_compr_az.c.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/benchmark/assets/lv_font_bechmark_montserrat_16_compr_az.c.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/benchmark/assets/lv_font_bechmark_montserrat_28_compr_az.c.c
    
    # LVGL Music Demo Assets
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_btn_corner_large.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_btn_list_pause_large.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_btn_list_pause.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_btn_list_play_large.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_btn_list_play.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_btn_loop_large.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_btn_loop.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_btn_next_large.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_btn_next.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_btn_pause_large.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_btn_pause.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_btn_play_large.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_btn_play.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_btn_prev_large.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_btn_prev.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_btn_rnd_large.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_btn_rnd.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_corner_left_large.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_corner_left.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_corner_right_large.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_corner_right.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_cover_1_large.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_cover_1.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_cover_2_large.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_cover_2.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_cover_3_large.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_cover_3.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_icon_1_large.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_icon_1.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_icon_2_large.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_icon_2.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_icon_3_large.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_icon_3.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_icon_4_large.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_icon_4.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_list_border_large.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_list_border.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_logo.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_slider_knob_large.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_slider_knob.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_wave_bottom_large.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_wave_bottom.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_wave_top_large.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets/img_lv_demo_music_wave_top.c
    
    # LVGL Source Files - Font 
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/font/lv_font_montserrat_8.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/font/lv_font_montserrat_10.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/font/lv_font_montserrat_12.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/font/lv_font_montserrat_14.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/font/lv_font_montserrat_16.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/font/lv_font_montserrat_18.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/font/lv_font_montserrat_20.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/font/lv_font_montserrat_22.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/font/lv_font_montserrat_24.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/font/lv_font_montserrat_26.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/font/lv_font_montserrat_28.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/font/lv_font_montserrat_30.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/font/lv_font_montserrat_32.c

    ${CMAKE_SOURCE_DIR}/Applications/system_task.c

    # Launcher Application
    ${CMAKE_SOURCE_DIR}/Applications/Launcher/launcher_task.c
    ${CMAKE_SOURCE_DIR}/Applications/Launcher/launcher_ui.c
    ${CMAKE_SOURCE_DIR}/Applications/Launcher/img_2048.c
    ${CMAKE_SOURCE_DIR}/Applications/Launcher/img_camera.c
    ${CMAKE_SOURCE_DIR}/Applications/Launcher/img_file.c
    ${CMAKE_SOURCE_DIR}/Applications/Launcher/img_osu.c
    ${CMAKE_SOURCE_DIR}/Applications/Launcher/img_settings.c
    ${CMAKE_SOURCE_DIR}/Applications/Launcher/img_recoder.c
    ${CMAKE_SOURCE_DIR}/Applications/Launcher/img_reader.c
    ${CMAKE_SOURCE_DIR}/Applications/Launcher/img_musicplay.c
    ${CMAKE_SOURCE_DIR}/Applications/Launcher/img_aichat.c

    # Music Game Application
    ${CMAKE_SOURCE_DIR}/Applications/MusicGame/music_game_core.c
    ${CMAKE_SOURCE_DIR}/Applications/MusicGame/music_game_task.c
    ${CMAKE_SOURCE_DIR}/Applications/MusicGame/music_game_ui.c

    # Camera Application
    ${CMAKE_SOURCE_DIR}/Applications/CameraApp/camera_task.c
    ${CMAKE_SOURCE_DIR}/Applications/CameraApp/camera_ui.c

    # Game 2048 Application
    ${CMAKE_SOURCE_DIR}/Applications/Game_2048/game_2048_core.c
    ${CMAKE_SOURCE_DIR}/Applications/Game_2048/game_2048_task.c
    ${CMAKE_SOURCE_DIR}/Applications/Game_2048/game_2048_ui.c

    # File Browser Application
    ${CMAKE_SOURCE_DIR}/Applications/FileBrowser/file_logic.c
    ${CMAKE_SOURCE_DIR}/Applications/FileBrowser/file_ui.c
    ${CMAKE_SOURCE_DIR}/Applications/FileBrowser/file_task.c

    # Settings Application
    ${CMAKE_SOURCE_DIR}/Applications/Settings/settings_task.c
    ${CMAKE_SOURCE_DIR}/Applications/Settings/settings_ui.c

    ${CMAKE_SOURCE_DIR}/Applications/Reader/reader_core.c
    ${CMAKE_SOURCE_DIR}/Applications/Reader/reader_task.c
    ${CMAKE_SOURCE_DIR}/Applications/Reader/reader_ui.c

)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    ${CMAKE_SOURCE_DIR}/BSP
    ${CMAKE_SOURCE_DIR}/BSP/DEBUG
    ${CMAKE_SOURCE_DIR}/BSP/FMC
    ${CMAKE_SOURCE_DIR}/BSP/GPIO
    ${CMAKE_SOURCE_DIR}/BSP/I2S
    ${CMAKE_SOURCE_DIR}/BSP/JPEG
    ${CMAKE_SOURCE_DIR}/BSP/LTDC
    ${CMAKE_SOURCE_DIR}/BSP/QSPI
    ${CMAKE_SOURCE_DIR}/BSP/QSPI/FontBin
    ${CMAKE_SOURCE_DIR}/BSP/SDIO
    ${CMAKE_SOURCE_DIR}/BSP/SDIO/FatFs
    ${CMAKE_SOURCE_DIR}/BSP/SPI
    ${CMAKE_SOURCE_DIR}/BSP/USB
    ${CMAKE_SOURCE_DIR}/BSP/USB/STM32_USB_Device_Library/Core/Inc
    ${CMAKE_SOURCE_DIR}/BSP/USB/STM32_USB_Device_Library/Class/CDC/Inc
    ${CMAKE_SOURCE_DIR}/BSP/USB/STM32_USB_Device_Library/Class/DFU/Inc
    ${CMAKE_SOURCE_DIR}/BSP/USB/STM32_USB_Device_Library/Class/MSC/Inc
    ${CMAKE_SOURCE_DIR}/BSP/USB/STM32_USB_Host_Library/Core/Inc
    ${CMAKE_SOURCE_DIR}/BSP/USB/STM32_USB_Host_Library/Class/HID/Inc

    # LVGL Include Paths
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/core
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/layouts/flex
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/layouts/grid
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/font
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/hal
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/misc
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/widgets
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/porting
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/benchmark
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/benchmark/assets
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/keypad_encoder
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/music/assets
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/stress
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/widgets
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/demos/widgets/assets
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/sw
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/draw/stm32_dma2d
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/layouts
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/layouts/flex
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/layouts/grid
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/animimg
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/calendar
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/chart
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/colorwheel
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/imgbtn
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/keyboard
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/led
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/list
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/menu
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/meter
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/msgbox
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/span
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/spinbox
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/spinner
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/tabview
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/tileview
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/widgets/win
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/themes
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/themes/default
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/themes/basic
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/themes/mono
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/others
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/others/fragment
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/others/gridnav
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/others/imgfont
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/others/monkey
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/others/msg
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/others/snapshot
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/libs
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/libs/bmp
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/libs/png
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/libs/gif
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/libs/qrcode
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/libs/sjpg
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/libs/fsdrv
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/libs/freetype
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/libs/ffmpeg
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/libs/rlottie
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/lvgl-8.3.11/src/extra/libs/tiny_ttf
    ${CMAKE_SOURCE_DIR}/Applications
    ${CMAKE_SOURCE_DIR}/Applications/CameraApp
    ${CMAKE_SOURCE_DIR}/Applications/Launcher
    ${CMAKE_SOURCE_DIR}/Applications/MusicGame
    ${CMAKE_SOURCE_DIR}/Applications/Settings
    ${CMAKE_SOURCE_DIR}/Applications/FileBrowser
    ${CMAKE_SOURCE_DIR}/Applications/Game_2048
    ${CMAKE_SOURCE_DIR}/Applications/Reader
    ${CMAKE_SOURCE_DIR}/Applications/AIChat

)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)