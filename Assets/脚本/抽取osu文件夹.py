import os
import re
import shutil
import random
from pathlib import Path

def filter_and_copy_4k_maps(source_dir, target_dir, count_to_pick):
    source_path = Path(source_dir)
    target_path = Path(target_dir)
    
    # 确保目标文件夹存在
    target_path.mkdir(parents=True, exist_ok=True)
    
    qualified_folders = []

    print("正在扫描文件夹，请稍候...")

    # 1. 遍历子文件夹
    # 获取 source_dir 下的一级子目录
    subfolders = [f for f in source_path.iterdir() if f.is_dir()]

    for folder in subfolders:
        osu_files = list(folder.glob("*.osu"))
        
        # 如果文件夹里根本没有 .osu 文件，跳过
        if not osu_files:
            continue
            
        all_is_4k = True
        
        # 2. 检查该文件夹下所有的 .osu 文件
        for osu_file in osu_files:
            try:
                with open(osu_file, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()
                    # 使用正则表达式查找 CircleSize 这一行
                    match = re.search(r'^CircleSize\s*:\s*(\d+)', content, re.MULTILINE)
                    
                    if match:
                        cs_value = match.group(1)
                        if cs_value != "4":
                            all_is_4k = False
                            break
                    else:
                        # 如果没找到 CircleSize 定义，通常不认为是标准 4K 谱面
                        all_is_4k = False
                        break
            except Exception as e:
                print(f"读取文件错误 {osu_file}: {e}")
                all_is_4k = False
                break
        
        # 3. 如果通过了“全员 4K”检查，加入备选列表
        if all_is_4k:
            qualified_folders.append(folder)

    print(f"扫描完成。符合全员 4K 条件的文件夹共有: {len(qualified_folders)} 个")

    # 4. 随机抽取
    if len(qualified_folders) < count_to_pick:
        print(f"警告: 符合条件的文件夹数量({len(qualified_folders)})不足 {count_to_pick} 个。将全部拷贝。")
        picked_folders = qualified_folders
    else:
        picked_folders = random.sample(qualified_folders, count_to_pick)

    # 5. 执行拷贝
    print(f"正在开始拷贝 {len(picked_folders)} 个文件夹...")
    for folder in picked_folders:
        dest_folder = target_path / folder.name
        try:
            # 如果目标已存在则删除（可选，为了干净）
            if dest_folder.exists():
                shutil.rmtree(dest_folder)
            shutil.copytree(folder, dest_folder)
            print(f"已拷贝: {folder.name}")
        except Exception as e:
            print(f"拷贝失败 {folder.name}: {e}")

    print("\n任务完成！")

if __name__ == "__main__":
    # --- 配置区域 ---
    SRC = "./Songs"           # 原始 osu! Songs 目录
    DST = "./Random_4K_Maps"  # 筛选后的存放目录
    NUM = 10                  # 你想要随机抽取的数量
    
    filter_and_copy_4k_maps(SRC, DST, NUM)