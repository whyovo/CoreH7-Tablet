import os
import re
from pathlib import Path
from PIL import Image
from pydub import AudioSegment

def process_osu_assets(input_dir, output_dir):
    input_path = Path(input_dir)
    output_path = Path(output_dir)

    # 支持的文件后缀
    audio_exts = {'.mp3', '.ogg', '.wav'}
    image_exts = {'.jpg', '.jpeg', '.png', '.bmp'}

    for root, dirs, files in os.walk(input_path):
        # 1. 在输出目录创建相同的子目录结构
        rel_path = Path(root).relative_to(input_path)
        target_dir = output_path / rel_path
        target_dir.mkdir(parents=True, exist_ok=True)

        for file in files:
            file_path = Path(root) / file
            ext = file_path.suffix.lower()
            
            # --- 处理音频 ---
            if ext in audio_exts:
                try:
                    target_file = target_dir / (file_path.stem + ".wav")
                    print(f"正在处理音频: {file}")
                    audio = AudioSegment.from_file(file_path)
                    # 设置为 44.1kHz, 16-bit (sample_width=2 bytes)
                    audio = audio.set_frame_rate(44100).set_sample_width(2).set_channels(2)
                    audio.export(target_file, format="wav")
                except Exception as e:
                    print(f"音频处理失败 {file}: {e}")

            # --- 处理图片 ---
            elif ext in image_exts:
                try:
                    target_file = target_dir / (file_path.stem + ".png")
                    print(f"正在处理图片: {file}")
                    img = Image.open(file_path).convert("RGB")
                    
                    # 目标尺寸 800x480
                    target_w, target_h = 800, 480
                    orig_w, orig_h = img.size
                    
                    # 缩放逻辑：先按比例缩放到高度为 480
                    scale_ratio = target_h / orig_h
                    inter_w = int(orig_w * scale_ratio)
                    img_resized = img.resize((inter_w, target_h), Image.Resampling.LANCZOS)
                    
                    # 裁剪逻辑：如果宽度超过 800，从中间截掉两边
                    if inter_w > target_w:
                        left = (inter_w - target_w) / 2
                        right = left + target_w
                        img_final = img_resized.crop((left, 0, right, target_h))
                    else:
                        img_final = img_resized
                    
                    img_final.save(target_file, "PNG")
                except Exception as e:
                    print(f"图片处理失败 {file}: {e}")

            # --- 处理 .osu 文件 ---
            elif ext == ".osu":
                target_file = target_dir / file
                print(f"正在修改谱面文件: {file}")
                with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()

                # 修改 AudioFilename: 字段
                content = re.sub(r'(AudioFilename\s*:\s*)(.*)\.(mp3|ogg|wav)', r'\1\2.wav', content, flags=re.IGNORECASE)
                
                # 修改 [Events] 里的背景图片引用 (通常是 0,0,"image.jpg",0,0)
                # 匹配引号内以图片后缀结尾的文件名，改为 .png
                for img_ext in image_exts:
                    pattern = rf'({re.escape(img_ext)})'
                    content = re.sub(r'(\.jpg|\.jpeg|\.bmp)', '.png', content, flags=re.IGNORECASE)

                with open(target_file, 'w', encoding='utf-8') as f:
                    f.write(content)

            # --- 其他文件 (如 .osb 等) 直接复制或忽略 ---
            else:
                # 如果需要同步其他文件，可以在这里写 shutil.copy2
                pass

if __name__ == "__main__":
    # 配置路径
    source_folder = r"D:\osu!\Songs\1701660 Various Artists - Malody 4K Regular Dan v3-Jack"      # 你的原始 osu! Songs 目录
    output_folder = r"E:\CODE_b\stm32ai\u盘备份\music_game\songs"   # 处理后的输出目录
    
    process_osu_assets(source_folder, output_folder)
    print("\n所有任务已完成！")