import os
from PIL import Image
import sys

def resize_images(input_folder, output_folder, max_size=480):
    """
    将图片文件夹中的所有jpg文件按比例裁剪到最长边480像素
    
    Args:
        input_folder: 输入图片文件夹路径
        output_folder: 输出图片文件夹路径
        max_size: 最大边长像素数(默认480)
    """
    
    # 创建输出文件夹
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)
        print(f"✓ 创建输出文件夹: {output_folder}")
    
    # 获取所有jpg文件
    jpg_files = [f for f in os.listdir(input_folder) 
                 if f.lower().endswith(('.jpg', '.jpeg'))]
    
    if not jpg_files:
        print(f"✗ 在 {input_folder} 中未找到jpg文件")
        return
    
    print(f"\n找到 {len(jpg_files)} 个jpg文件，开始处理...\n")
    
    for idx, filename in enumerate(jpg_files, 1):
        try:
            input_path = os.path.join(input_folder, filename)
            output_path = os.path.join(output_folder, filename)
            
            # 打开图片
            img = Image.open(input_path)
            original_size = img.size  # (width, height)
            
            # 计算缩放比例
            max_dimension = max(original_size)
            scale_ratio = max_size / max_dimension
            
            # 计算新尺寸(保持宽高比)
            new_width = int(original_size[0] * scale_ratio)
            new_height = int(original_size[1] * scale_ratio)
            
            # 使用高质量重采样方法
            img_resized = img.resize((new_width, new_height), Image.Resampling.LANCZOS)
            
            # 保存图片
            img_resized.save(output_path, quality=95)
            
            print(f"[{idx:3d}/{len(jpg_files)}] ✓ {filename}")
            print(f"         原尺寸: {original_size[0]}×{original_size[1]} px")
            print(f"         新尺寸: {new_width}×{new_height} px (比例: {scale_ratio:.2%})")
            
        except Exception as e:
            print(f"[{idx:3d}/{len(jpg_files)}] ✗ {filename} - 错误: {str(e)}")
    
    print(f"\n✓ 处理完成！输出文件夹: {output_folder}")

def main():
    """主函数"""
    # 设置路径
    base_path = r"E:\CODE_b\stm32ai\u盘备份"
    jpg_folder = base_path
    output_folder = os.path.join(base_path, "jpg_resized")
    
    # 检查输入文件夹
    if not os.path.exists(jpg_folder):
        print(f"✗ 输入文件夹不存在: {jpg_folder}")
        sys.exit(1)
    
    print("=" * 60)
    print("  图片裁剪工具 - 按比例缩放到最长边480像素")
    print("=" * 60)
    print(f"输入文件夹: {jpg_folder}")
    print(f"输出文件夹: {output_folder}")
    print()
    
    # 执行裁剪
    resize_images(jpg_folder, output_folder, max_size=480)

if __name__ == "__main__":
    main()