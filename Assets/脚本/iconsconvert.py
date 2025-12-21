import os
from PIL import Image
from collections import deque


def rgb888_to_rgb565(r, g, b):
    # 将 888 转换为 565 格式 (Little Endian 存储准备)
    # RRRRRGGG GGGBBBBB
    be_val = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
    return be_val & 0xFF, (be_val >> 8) & 0xFF


def process_images(
    input_dir, mode, target_size=(120, 120), tolerance=70, expand_pixels=1
):
    if not os.path.exists(input_dir):
        print(f"错误: 路径 '{input_dir}' 不存在")
        return

    # 逻辑配置
    do_bg_removal = False
    in_exts = (".png", ".jpg", ".jpeg")

    if mode == "1":
        out_ext = ".jpg"
        print(f"-> 启动模式 1: [背景填充] PNG 转高质 JPG")
    elif mode == "2":
        out_ext = ".png"
        do_bg_removal = True
        print(f"-> 启动模式 2: [智能抠图] JPG 转透明 PNG (容差:{tolerance})")
    elif mode == "3":
        out_ext = ".c"
        do_bg_removal = True  # 默认开启抠图以防源码是带白边的JPG
        print(f"-> 启动模式 3: [嵌入式源码] 转 LVGL C数组 (RGB565 + Alpha)")
    elif mode == "4":
        out_ext = ".png"
        do_bg_removal = False  # 直接缩放，保留原有的透明度
        print(
            f"-> 启动模式 4: [图标标准化] 任意尺寸统一转为 {target_size[0]}x{target_size[1]} 透明 PNG"
        )
    else:
        print("无效模式，退出。")
        return

    for root, dirs, files in os.walk(input_dir):
        for file in files:
            if file.lower().endswith(in_exts):
                file_path = os.path.join(root, file)
                try:
                    with Image.open(file_path) as img:
                        # 1. 尺寸归一化与居中裁剪 (Center Crop)
                        w, h = img.size
                        ratio = max(target_size[0] / w, target_size[1] / h)
                        new_size = (int(w * ratio), int(h * ratio))
                        img = img.resize(new_size, Image.Resampling.LANCZOS)

                        left = (new_size[0] - target_size[0]) / 2
                        top = (new_size[1] - target_size[1]) / 2
                        img = img.crop(
                            (left, top, left + target_size[0], top + target_size[1])
                        )

                        img = img.convert("RGBA")

                        # 2. 抠图逻辑 (Flood Fill 算法)
                        if do_bg_removal:
                            width, height = img.size
                            pixels = img.load()
                            is_bg = [
                                [False for _ in range(height)] for _ in range(width)
                            ]
                            visited = set()
                            queue = deque()

                            def is_white_enough(x, y):
                                r, g, b, _ = pixels[x, y]
                                return (
                                    (255 - r) ** 2 + (255 - g) ** 2 + (255 - b) ** 2
                                ) ** 0.5 < tolerance

                            # 边界扫描
                            for x in range(width):
                                for y in [0, height - 1]:
                                    if is_white_enough(x, y):
                                        queue.append((x, y))
                            for y in range(height):
                                for x in [0, width - 1]:
                                    if is_white_enough(x, y):
                                        queue.append((x, y))

                            while queue:
                                cx, cy = queue.popleft()
                                if (cx, cy) in visited:
                                    continue
                                visited.add((cx, cy))
                                is_bg[cx][cy] = True
                                for dx, dy in [
                                    (-1, 0),
                                    (1, 0),
                                    (0, -1),
                                    (0, 1),
                                    (-1, -1),
                                    (-1, 1),
                                    (1, -1),
                                    (1, 1),
                                ]:
                                    nx, ny = cx + dx, cy + dy
                                    if (
                                        0 <= nx < width
                                        and 0 <= ny < height
                                        and is_white_enough(nx, ny)
                                    ):
                                        if (nx, ny) not in visited:
                                            queue.append((nx, ny))

                            # 蚕食边缘 (针对白边优化)
                            if expand_pixels > 0:
                                for _ in range(expand_pixels):
                                    to_add = []
                                    for x in range(1, width - 1):
                                        for y in range(1, height - 1):
                                            if not is_bg[x][y] and any(
                                                is_bg[x + dx][y + dy]
                                                for dx, dy in [
                                                    (-1, 0),
                                                    (1, 0),
                                                    (0, -1),
                                                    (0, 1),
                                                ]
                                            ):
                                                to_add.append((x, y))
                                    for ax, ay in to_add:
                                        is_bg[ax][ay] = True

                            for x in range(width):
                                for y in range(height):
                                    if is_bg[x][y]:
                                        r, g, b, _ = pixels[x, y]
                                        pixels[x, y] = (r, g, b, 0)

                        # 3. 输出保存
                        base_name = os.path.splitext(file)[0]
                        if mode == "1":
                            img.convert("RGB").save(
                                os.path.join(root, base_name + ".jpg"), quality=100
                            )
                        elif mode in ["2", "4"]:
                            img.save(os.path.join(root, base_name + ".png"))
                        elif mode == "3":
                            # 生成 C 数组 (RGB565 + Alpha)
                            c_path = os.path.join(root, f"img_{base_name}.c")
                            h_path = os.path.join(root, f"img_{base_name}.h")
                            with open(c_path, "w") as f_c:
                                f_c.write('#include "lvgl.h"\n\n')
                                f_c.write(
                                    f"const uint8_t img_{base_name}_map[] = {{\n  "
                                )
                                pixel_data = img.getdata()
                                for i, (r, g, b, a) in enumerate(pixel_data):
                                    low, high = rgb888_to_rgb565(r, g, b)
                                    f_c.write(f"0x{low:02x}, 0x{high:02x}, 0x{a:02x}, ")
                                    if (i + 1) % 4 == 0:
                                        f_c.write("\n  ")
                                f_c.write("\n};\n\n")
                                f_c.write(f"const lv_img_dsc_t img_{base_name} = {{\n")
                                f_c.write(
                                    f"  .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,\n"
                                )
                                f_c.write(f"  .header.always_zero = 0,\n")
                                f_c.write(f"  .header.w = {target_size[0]},\n")
                                f_c.write(f"  .header.h = {target_size[1]},\n")
                                f_c.write(
                                    f"  .data_size = {target_size[0] * target_size[1] * 3},\n"
                                )
                                f_c.write(f"  .data = img_{base_name}_map,\n}};\n")
                            with open(h_path, "w") as f_h:
                                f_h.write(
                                    f"#ifndef IMG_{base_name.upper()}_H\n#define IMG_{base_name.upper()}_H\n\n"
                                )
                                f_h.write('#include "lvgl.h"\n\n')
                                f_h.write(
                                    f"extern const lv_img_dsc_t img_{base_name};\n\n#endif\n"
                                )

                        print(f"已处理完成: {file}")

                except Exception as e:
                    print(f"!!! 无法处理 {file}: {e}")


if __name__ == "__main__":
    print("\n" + "=" * 50)
    print("      STM32 / LVGL 图标自动化处理脚本 V2.0")
    print("=" * 50)
    print(" [1] 背景填充模式: PNG 转 JPG ")
    print("     - 自动缩放裁剪，无透明度，体积小。")
    print("\n [2] 智能抠图模式: JPG 转透明 PNG (用于普通图标自制)")
    print("     - 识别边界白色并转为透明，自动蚕食边缘减少白边。")
    print("\n [3] 源码生成模式: 转为 LVGL C语言数组 (用于固件内置)")
    print("     - 格式: RGB565 + Alpha (True Color Alpha)。")
    print("     - 会自动执行抠图逻辑。")
    print("\n [4] 图标标准化模式: 任意png尺寸转 120x120 透明 PNG")
    print("     - 纯粹的缩放居中裁剪，不改变原有透明信息。")
    print("     - 适合原本就是透明的素材进行统一尺寸处理。")
    print("=" * 50)

    choice = input("\n请选择工作模式 (1-4): ").strip()
    path = input("请输入素材文件夹路径: ").strip('"').strip()

    # 执行处理
    process_images(path, choice, target_size=(120, 120), tolerance=70, expand_pixels=1)

    print("\n" + "=" * 50)
    print(" 所有任务执行完毕！")
    print("=" * 50)
