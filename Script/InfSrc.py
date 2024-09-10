# usage: py -3 InfSrc.py ./path_to_src_dir .c

import os
import argparse

def generate_inf(target_dir, file_extensions):
    # 获取当前工作目录的绝对路径
    current_dir = os.getcwd()

    # 将传入的 target_dir 转换为绝对路径
    target_dir = os.path.abspath(target_dir)

    # 获取 .py 文件所在目录，生成的 INF 文件放在这个目录下
    script_dir = os.path.dirname(os.path.realpath(__file__))
    inf_file = os.path.join(script_dir, "sources.inf")

    # 将文件后缀全部转换为小写以进行不区分大小写的比较
    file_extensions = [ext.lower() for ext in file_extensions]

    # 检查目标目录是否存在
    if not os.path.exists(target_dir):
        print(f"Error: Target directory {target_dir} does not exist.")
    else:
        # 初始化存放相对路径的文件列表
        source_files = []

        # 递归遍历 target_dir 及其子目录
        for root, dirs, files in os.walk(target_dir):
            for file in files:
                # 获取文件后缀并转换为小写
                file_ext = os.path.splitext(file)[1].lower()
                if file_ext in file_extensions:
                    # 计算相对路径并替换反斜杠为正斜杠
                    relative_path = os.path.relpath(os.path.join(root, file), current_dir).replace("\\", "/")
                    source_files.append(relative_path)

        if len(source_files) == 0:
            print(f"No files with extensions {file_extensions} found in the directory.")
        else:
            # 将文件的相对路径写入 INF 文件的 [Sources] 段
            with open(inf_file, 'w') as f:
                f.write("[Sources]\n")
                for source_file in source_files:
                    f.write(f"{source_file}\n")

            print(f"INF file {inf_file} has been updated with {len(source_files)} source files.")

if __name__ == "__main__":
    # 设置参数解析
    parser = argparse.ArgumentParser(description="Generate INF file with specific file extensions.")
    parser.add_argument('target_dir', type=str, help='Target directory to scan for source files')
    parser.add_argument('file_extensions', nargs='+', type=str, help='List of file extensions to look for (e.g., .c .S .h)')

    # 解析参数
    args = parser.parse_args()

    # 生成 INF 文件
    generate_inf(args.target_dir, args.file_extensions)
