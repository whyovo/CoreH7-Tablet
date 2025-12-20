import os
from PIL import Image
from collections import deque


def rgb888_to_rgb565(r, g, b):
    # 将 888 转换为 565 格式 (Little Endian 存储准备)
    # RRRRRGGG GGGBBBBB
    be_val = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
    # 返回低字节和高字节 (STM32 为小端序)
    return be_val & 0xFF, (be_val >> 8) & 0xFF


def process_images(
    input_dir, mode, target_size=(120, 120), tolerance=70, expand_pixels=1
):
    if not os.path.exists(input_dir):
        print(f"错误: 路径 '{input_dir}' 不存在")
        return

    # 模式设置
    if mode == "3":
        in_ext, out_ext = ".png", ".c"
    else:
        in_ext, out_ext = (".png", ".jpg") if mode == "1" else (".jpg", ".png")

    print(f"模式 {mode} 启动 | 容差: {tolerance} | 蚕食像素: {expand_pixels}")

    for root, dirs, files in os.walk(input_dir):
        for file in files:
            if file.lower().endswith(in_ext):
                file_path = os.path.join(root, file)
                try:
                    with Image.open(file_path) as img:
                        # 1. 尺寸归一化与裁剪
                        w, h = img.size
                        ratio = max(target_size[0] / w, target_size[1] / h)
                        new_size = (int(w * ratio), int(h * ratio))
                        img = img.resize(new_size, Image.Resampling.LANCZOS)
                        left, top = (new_size[0] - target_size[0]) / 2, (
                            new_size[1] - target_size[1]
                        ) / 2
                        img = img.crop(
                            (left, top, left + target_size[0], top + target_size[1])
                        )

                        # 2. 核心处理：抠图逻辑 (仅模式2和3需要)
                        img = img.convert("RGBA")
                        if mode in ["2", "3"]:
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
                        elif mode == "2":
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
                                    f"extern const lv_img_dsc_t img_{base_name};\n\n"
                                )
                                f_h.write("#endif\n")

                        print(f"完成: {file}")

                except Exception as e:
                    print(f"跳过 {file}: {e}")


if __name__ == "__main__":
    print("模式选择: 1-JPG, 2-透明PNG, 3-LVGL C数组(16bit+Alpha)")
    choice = input("请输入模式: ").strip()
    path = input("请输入路径: ").strip('"').strip()
    process_images(path, choice, tolerance=70, expand_pixels=1)
