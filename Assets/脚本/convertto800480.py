import os
from PIL import Image, ImageOps

def process_images_recursive(root_dir, target_size=(800, 480), output_root_name="output_images"):
    """
    递归遍历目录，将图片裁剪并缩放到指定大小 (中心裁剪)
    :param root_dir: 输入的根目录
    :param target_size: 目标分辨率 (宽, 高)
    :param output_root_name: 输出文件夹的名字
    """
    
    # 统计计数
    count = 0
    error_count = 0

    # 确保输出根目录存在
    if not os.path.exists(output_root_name):
        os.makedirs(output_root_name)

    print(f"开始处理: {root_dir}")
    print(f"目标尺寸: {target_size[0]}x{target_size[1]}")
    print("-" * 30)

    # os.walk 递归遍历所有子目录
    for dirpath, dirnames, filenames in os.walk(root_dir):
        for filename in filenames:
            # 检查后缀名 (不区分大小写)
            if filename.lower().endswith(('.jpg', '.jpeg')):
                
                file_path = os.path.join(dirpath, filename)
                
                # 构建输出路径，保持原有目录结构
                # 计算相对路径，例如: input/A/1.jpg -> A/1.jpg
                rel_path = os.path.relpath(dirpath, root_dir)
                
                # 拼接输出文件夹: output/A
                output_dir_path = os.path.join(output_root_name, rel_path)
                
                if not os.path.exists(output_dir_path):
                    os.makedirs(output_dir_path)
                
                output_file_path = os.path.join(output_dir_path, filename)

                try:
                    with Image.open(file_path) as img:
                        # 转换颜色模式，防止 RGBA 格式存为 JPG 时报错
                        if img.mode != 'RGB':
                            img = img.convert('RGB')

                        # 核心逻辑：ImageOps.fit
                        # method: 缩放算法，LANCZOS 是高质量缩放
                        # centering: (0.5, 0.5) 表示从正中心裁剪 (0,0是左上, 1,1是右下)
                        new_img = ImageOps.fit(
                            img, 
                            target_size, 
                            method=Image.Resampling.LANCZOS, 
                            centering=(0.5, 0.5)
                        )

                        # 保存图片，质量设为95
                        new_img.save(output_file_path, quality=95)
                        print(f"[OK] {filename} -> {target_size}")
                        count += 1

                except Exception as e:
                    print(f"[Error] 处理 {filename} 失败: {e}")
                    error_count += 1

    print("-" * 30)
    print(f"处理完成！")
    print(f"成功: {count} 张")
    print(f"失败: {error_count} 张")
    print(f"图片已保存至: {os.path.abspath(output_root_name)}")

if __name__ == '__main__':
    # ================= 配置区域 =================
    
    # 请在这里修改你要搜索的文件夹路径
    # 比如: r"C:\Users\Admin\Pictures\Wallpapers"
    INPUT_DIRECTORY = r"E:\CODE_b\stm32ai\u盘备份\不需要保存的文件\backgroundraw"
    
    TARGET_WIDTH = 800
    TARGET_HEIGHT = 480
    
    # ===========================================

    # 检查输入目录是否存在
    if os.path.exists(INPUT_DIRECTORY):
        process_images_recursive(INPUT_DIRECTORY, (TARGET_WIDTH, TARGET_HEIGHT))
    else:
        print(f"错误: 找不到目录 '{INPUT_DIRECTORY}'")
        print("请在代码底部的 'INPUT_DIRECTORY' 变量中修改路径。")