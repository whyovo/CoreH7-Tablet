import os
import re
import struct
from pathlib import Path
from PIL import Image, ImageOps
from pydub import AudioSegment

# 尝试导入星数计算库
try:
    import rosu_pp_py as rosu
except ImportError:
    rosu = None
    print("提示: 未安装 rosu-pp-py，建议运行 'pip install rosu-pp-py' 以获得准确星数。")


def sanitize_name(name):
    """移除非法字符"""
    return re.sub(r'[\\/*?:"<>|]', "", name).strip()


def truncate_string(text, max_len=80):
    """严格限制字符串长度，防止 Windows 路径过长报错"""
    return text[:max_len].strip() if len(text) > max_len else text


def analyze_beatmap(content):
    """解析谱面：BPM, 音符统计, 时长, 官方星数"""
    stats = {"BPM": 0, "ShortNotes": 0, "LongNotes": 0, "Stars": 0.0, "Duration": 0}

    lines = content.split("\n")
    section = ""
    first_tp = True
    first_obj_time = None
    last_obj_time = 0

    for line in lines:
        line = line.strip()
        if not line or line.startswith("//"):
            continue
        if line.startswith("["):
            section = line
            continue

        # 1. 解析 BPM (取第一个 TimingPoint)
        if section == "[TimingPoints]" and first_tp:
            parts = line.split(",")
            if len(parts) >= 2:
                try:
                    ms_per_beat = float(parts[1])
                    if ms_per_beat > 0:
                        stats["BPM"] = round(60000 / ms_per_beat, 1)
                    first_tp = False
                except:
                    pass

        # 2. 解析音符及计算时长
        if section == "[HitObjects]":
            parts = line.split(",")
            if len(parts) >= 4:
                try:
                    time = int(parts[2])
                    if first_obj_time is None:
                        first_obj_time = time
                    last_obj_time = max(last_obj_time, time)

                    if int(parts[3]) & 128:  # Mania 长条位
                        stats["LongNotes"] += 1
                        # 处理长条结束时间以校准时长
                        ln_end = int(parts[5].split(":")[0])
                        last_obj_time = max(last_obj_time, ln_end)
                    else:
                        stats["ShortNotes"] += 1
                except:
                    pass

    # 计算时长 (秒)
    if first_obj_time is not None:
        stats["Duration"] = (last_obj_time - first_obj_time) // 1000

    # 3. 官方星数计算 (修复后的 API 调用)
    if rosu:
        try:
            b_map = rosu.Beatmap(content=content)
            # rosu-pp 会自动根据 .osu 里的 Mode 识别为 Mania
            result = rosu.Difficulty().calculate(b_map)
            stats["Stars"] = round(result.stars, 2)
        except Exception as e:
            print(f"  !! 星数计算失败: {e}")

    # 保底星数估算 (NPS 算法)
    if stats["Stars"] <= 0 and stats["Duration"] > 0:
        nps = (stats["ShortNotes"] + stats["LongNotes"]) / stats["Duration"]
        stats["Stars"] = round(nps * 0.8, 2)

    return stats


def process_standalone_osu(input_dir, output_root):
    input_path = Path(input_dir)
    output_path = Path(output_root)
    osu_files = list(input_path.rglob("*.osu"))

    for osu_file in osu_files:
        if osu_file.stat().st_size < 4096:
            continue

        # 1. 预读取元数据
        try:
            with open(osu_file, "r", encoding="utf-8", errors="ignore") as f:
                content = f.read()
        except:
            continue

        meta = {
            "Title": "Unknown",
            "Version": "Normal",
            "Audio": None,
            "Background": None,
        }
        t_m = re.search(r"Title:(.*)", content)
        v_m = re.search(r"Version:(.*)", content)
        a_m = re.search(r"AudioFilename:(.*)", content)
        if t_m:
            meta["Title"] = t_m.group(1).strip()
        if v_m:
            meta["Version"] = v_m.group(1).strip()
        if a_m:
            meta["Audio"] = a_m.group(1).strip()

        ev_sec = re.search(r"\[Events\]\s*(.*?)\s*\[", content, re.DOTALL)
        if ev_sec:
            bg_m = re.search(r'0,0,"(.*?)"', ev_sec.group(1)) or re.search(
                r"0,0,([^,]*)", ev_sec.group(1)
            )
            if bg_m:
                meta["Background"] = bg_m.group(1).strip()

        # 2. 文件夹与文件名长度控制 (解决 Filename too long)
        safe_title = truncate_string(sanitize_name(meta["Title"]), 40)
        safe_ver = truncate_string(sanitize_name(meta["Version"]), 30)
        folder_name = f"{safe_title} [{safe_ver}]"

        target_dir = output_path / folder_name
        target_dir.mkdir(parents=True, exist_ok=True)

        # .osu 文件名也进行截断处理
        safe_osu_name = truncate_string(sanitize_name(osu_file.stem), 50) + ".osu"

        print(f"\n[处理项目] {folder_name}")

        # 3. 统计难度数据
        stats = analyze_beatmap(content)

        # 4. 转换资源 (标准化命名：audio.wav / bg.jpg)
        if meta["Audio"]:
            src = osu_file.parent / meta["Audio"]
            if src.exists():
                dest = target_dir / "audio.wav"
                if not dest.exists():
                    try:
                        AudioSegment.from_file(src).set_frame_rate(
                            44100
                        ).set_sample_width(2).set_channels(2).export(dest, format="wav")
                    except:
                        pass

        if meta["Background"]:
            src = osu_file.parent / meta["Background"]
            if src.exists():
                dest = target_dir / "bg.jpg"
                if not dest.exists():
                    try:
                        with Image.open(src) as img:
                            ImageOps.fit(
                                img.convert("RGB"), (800, 480), centering=(0.5, 0.5)
                            ).save(dest, "JPEG", quality=90)
                    except:
                        pass

        # 5. 数据注入与引用修正
        nps = (
            round((stats["ShortNotes"] + stats["LongNotes"]) / stats["Duration"], 1)
            if stats["Duration"] > 0
            else 0
        )
        inject_data = (
            f"\r\n//--- STM32 Optimized Data ---\r\n"
            f"Stars: {stats['Stars']}\r\n"
            f"BPM: {stats['BPM']}\r\n"
            f"Duration: {stats['Duration']}s\r\n"
            f"NPS: {nps}\r\n"
            f"ShortNotes: {stats['ShortNotes']}\r\n"
            f"LongNotes: {stats['LongNotes']}\r\n"
        )

        content = re.sub(
            r"(AudioFilename\s*:\s*).*", r"\1audio.wav", content, flags=re.IGNORECASE
        )
        if meta["Background"]:
            content = content.replace(meta["Background"], "bg.jpg")
        content = content.replace("[Difficulty]", inject_data + "\r\n[Difficulty]")

        with open(target_dir / safe_osu_name, "w", encoding="utf-8") as f:
            f.write(content)


if __name__ == "__main__":
    # 配置你的路径
    # --- 配置区域 ---
    # 输入：osu! 原始 Songs 路径
    INPUT_DIR = r"D:\osu!\Songs\1701660 Various Artists - Malody 4K Regular Dan v3-Jack"
    # 输出：STM32 专用的打包目录
    OUTPUT_DIR = r"E:\CODE_b\stm32ai\4k_music_game\Assets\U盘内容\music_game\songs"

    if os.path.exists(INPUT_DIR):
        process_standalone_osu(INPUT_DIR, OUTPUT_DIR)
        print("\n所有谱面处理完成！")
