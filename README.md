# CoreH7-Tablet 项目

基于 STM32H750XBHx 的平板开发框架，目前还在写，readme作占位

![1](.\Assets\readme_pic\1.jpg)

![2](.\Assets\readme_pic\2.jpg)

![3](.\Assets\readme_pic\3.jpg)

![4](.\Assets\readme_pic\4.jpg)

需要准备一张sd卡，把CoreH7-Tablet\Assets\U盘内容里面的所有文件夹拷贝到sd卡，并正确安装到单片机里面。具体见原理图CoreH7-Tablet\Assets\Docs\原理图和机械尺寸



## 项目结构

- [Applications/](Applications/): 顶层应用逻辑，包括 [Launcher](Applications/Launcher/)、[Game_2048](Applications/Game_2048/) 等。
- [Bootloader/](Bootloader/): 引导加载程序，负责系统启动和 QSPI Flash 管理。
- [BSP/](BSP/): 板级支持包，包含 [SDRAM](BSP/FMC/sdram.c)、[LCD (LTDC)](4k_music_game.ioc)、[Audio](BSP/I2S/audio_player.c) 等驱动。
- [Middlewares/](Middlewares/): 第三方库，如 [LVGL 8.3.11](Middlewares/Third_Party/lvgl-8.3.11/)。
- [Core/](Core/): STM32CubeMX 生成的核心初始化代码。

