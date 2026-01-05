import os
from pydub import AudioSegment


def convert_audio_to_stm32_wav(input_dir, output_dir, target_sample_rate=44100):
    """
    递归搜索目录下的 mp3 和 wav，并标准化为 44.1kHz 16-bit WAV
    """
    # 允许的处理格式
    valid_extensions = (".mp3", ".wav")

    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    print(f"开始处理音频资源...")
    print(f"目标参数: {target_sample_rate}Hz, 16-bit, Stereo WAV")
    print("-" * 50)

    for root, dirs, files in os.walk(input_dir):
        for file in files:
            if file.lower().endswith(valid_extensions):
                input_path = os.path.join(root, file)

                # 保持子目录结构
                relative_path = os.path.relpath(root, input_dir)
                dest_folder = os.path.join(output_dir, relative_path)
                if not os.path.exists(dest_folder):
                    os.makedirs(dest_folder)

                # 生成输出文件名（统一改为 .wav）
                file_name_no_ext = os.path.splitext(file)[0]
                output_path = os.path.join(dest_folder, f"{file_name_no_ext}.wav")

                try:
                    # 1. 加载音频
                    audio = AudioSegment.from_file(input_path)

                    # 2. 设置参数：采样率、位深(16-bit)、通道(2-双声道)
                    # STM32H7 的 I2S 通常配置为 16-bit 数据长度
                    audio = audio.set_frame_rate(target_sample_rate)
                    audio = audio.set_sample_width(2)  # 2 字节 = 16 bit
                    audio = audio.set_channels(2)  # 立体声

                    # 3. 导出
                    audio.export(output_path, format="wav")
                    print(f"成功: {file} -> {os.path.relpath(output_path, output_dir)}")

                except Exception as e:
                    print(f"失败: 无法处理 {file}. 错误: {e}")


if __name__ == "__main__":
    # --- 配置区域 ---
    input_folder = input("请输入源音频文件夹路径: ").strip('"').strip()
    output_folder = "stm32_assets_audio"

    convert_audio_to_stm32_wav(input_folder, output_folder)

    print("-" * 50)
    print(f"处理完成！所有文件已存入: {output_folder}")
