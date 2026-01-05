import os
import struct
from PIL import Image
from collections import deque


def rgb888_to_rgb565(r, g, b):
    """将 RGB888 转换为 RGB565 格式 (小端序存储)"""
    # RRRRRGGG GGGBBBBB
    be_val = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
    # 返回字节流：低字节在前，高字节在后 (Little Endian)
    return struct.pack("<H", be_val)


def process_images(
    input_dir, mode, target_size=(120, 120), tolerance=70, expand_pixels=1
):
    if not os.path.exists(input_dir):
        print(f"错误: 路径 '{input_dir}' 不存在")
        return

    # 逻辑配置初始化
    do_bg_removal = False
    in_exts = (".png", ".jpg", ".jpeg")

    if mode == "1":
        print(f"-> 启动模式 1: [背景填充] PNG 转高质 JPG (统一缩放裁剪)")
    elif mode == "2":
        do_bg_removal = True
        print(f"-> 启动模式 2: [智能抠图] JPG 转透明 PNG (保持原尺寸)")
    elif mode == "3":
        do_bg_removal = True
        print(f"-> 启动模式 3: [嵌入式源码] 转 LVGL C数组 (保持原尺寸, RGB565+A)")
    elif mode == "4":
        print(f"-> 启动模式 4: [图标标准化] 统一转为 {target_size} 透明 PNG")
    elif mode == "5":
        do_bg_removal = True
        print(f"-> 启动模式 5: [二进制导出] 转 LVGL .bin 文件 (针对长条/大背景优化)")
    else:
        print("无效模式，退出。")
        return

    for root, dirs, files in os.walk(input_dir):
        for file in files:
            if file.lower().endswith(in_exts):
                file_path = os.path.join(root, file)
                try:
                    with Image.open(file_path) as img:
                        # --- 1. 尺寸处理逻辑 ---
                        if mode in ["2", "3", "5"]:
                            # 保持原始尺寸
                            out_w, out_h = img.size
                        else:
                            # 统一缩放并居中裁剪
                            out_w, out_h = target_size
                            w, h = img.size
                            ratio = max(out_w / w, out_h / h)
                            new_size = (int(w * ratio), int(h * ratio))
                            img = img.resize(new_size, Image.Resampling.LANCZOS)
                            left = (new_size[0] - out_w) / 2
                            top = (new_size[1] - out_h) / 2
                            img = img.crop((left, top, left + out_w, top + out_h))

                        img = img.convert("RGBA")
                        width, height = img.size

                        # --- 2. 智能抠图逻辑 (Flood Fill) ---
                        if do_bg_removal:
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

                            # 边界扫描起点
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
                                        and (nx, ny) not in visited
                                    ):
                                        if is_white_enough(nx, ny):
                                            queue.append((nx, ny))

                            # 蚕食边缘 (减少白边)
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

                        # --- 3. 输出保存逻辑 ---
                        base_name = (
                            os.path.splitext(file)[0]
                            .replace(" ", "_")
                            .replace("-", "_")
                        )

                        if mode == "1":
                            img.convert("RGB").save(
                                os.path.join(root, base_name + ".jpg"), quality=100
                            )

                        elif mode in ["2", "4"]:
                            img.save(os.path.join(root, base_name + ".png"))

                        elif mode == "3":
                            # 生成 C 数组
                            c_path = os.path.join(root, f"img_{base_name}.c")
                            h_path = os.path.join(root, f"img_{base_name}.h")
                            with open(c_path, "w", encoding="utf-8") as f:
                                f.write('#include "lvgl.h"\n\n')
                                f.write(f"const uint8_t img_{base_name}_map[] = {{\n  ")
                                for i, (r, g, b, a) in enumerate(img.getdata()):
                                    p565 = (
                                        ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
                                    )
                                    f.write(
                                        f"0x{p565&0xFF:02x}, 0x{p565>>8:02x}, 0x{a:02x}, "
                                    )
                                    if (i + 1) % 4 == 0:
                                        f.write("\n  ")
                                f.write(
                                    f"\n}};\n\nconst lv_img_dsc_t img_{base_name} = {{"
                                )
                                f.write(
                                    f".header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA, .header.w = {width}, .header.h = {height}, "
                                )
                                f.write(
                                    f".data_size = {width*height*3}, .data = img_{base_name}_map}};\n"
                                )

                            with open(h_path, "w", encoding="utf-8") as f:
                                f.write(
                                    f"#ifndef IMG_{base_name.upper()}_H\n#define IMG_{base_name.upper()}_H\n"
                                )
                                f.write('#include "lvgl.h"\n')
                                f.write(
                                    f"extern const lv_img_dsc_t img_{base_name};\n#endif\n"
                                )

                        elif mode == "5":
                            # 生成 LVGL 二进制文件 (.bin)
                            bin_path = os.path.join(root, f"{base_name}.bin")
                            with open(bin_path, "wb") as f:
                                # LVGL v8 Binary Header (4 bytes)
                                # 结构: CF(5bit) | AlwaysZero(3bit) | Width(11bit) | Height(11bit)

                                cf = 5

                                header = struct.pack(
                                    "<I",
                                    (cf & 0x1F)
                                    | ((width & 0x7FF) << 10)
                                    | ((height & 0x7FF) << 21),
                                )
                                f.write(header)

                                # 写入数据 (RGB565 + Alpha)
                                for r, g, b, a in img.getdata():
                                    f.write(rgb888_to_rgb565(r, g, b))  # 2字节颜色
                                    f.write(struct.pack("B", a))  # 1字节透明

                        print(f"成功: {file} -> {width}x{height} (模式{mode})")

                except Exception as e:
                    print(f"!!! 无法处理 {file}: {e}")


if __name__ == "__main__":
    print("\n" + "=" * 60)
    print("        STM32H7 / LVGL 图像全能处理工具 V2.3")
    print("=" * 60)
    print(" [1] 背景填充模式: PNG 转 JPG (用于 800x480 非透明背景)")
    print(" [2] 智能抠图模式: JPG 转透明 PNG (自动去白边)")
    print(" [3] 源码生成模式: 转为 .c/.h 数组 (用于小图标，存入 QSPI)")
    print(" [4] 图标标准化模式: 统一转为 120x120 透明 PNG")
    print(" [5] 二进制导出模式: 转为 .bin 文件 (存入 SD 卡)")
    print("=" * 60)

    choice = input("\n请选择模式 (1-5): ").strip()
    folder = input("请输入文件夹路径: ").strip('"').strip()

    process_images(folder, choice)
    print("\n任务全部完成！")
