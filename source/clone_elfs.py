import os
import re
import shutil
import sys


def list_subdirs(directory: str) -> list[str]:
    return next(os.walk(directory))[1]


def list_files(directory: str) -> list[str]:
    return next(os.walk(directory))[2]


def is_elf(filename: str) -> bool:
    return re.search(".*\\.elf", filename) is not None


def list_elfs_recursively(directory: str) -> list[str]:
    elfs = []
    for (subdir, _, file_names) in os.walk(directory):
        elfs = elfs + [subdir + "/" + elf for elf in filter(is_elf, file_names)]

    return elfs


def filter_elfs(elfs: list[str], requested: list[str]) -> list[str]:
    if len(requested) == 0:
        return elfs

    filtered = []
    for elf in elfs:
        for req in requested:
            if elf.endswith(req):
                filtered.append(elf)
    return filtered


def copy_file(src_path, dst_path):
    dst_dir = os.path.dirname(dst_path)
    if not os.path.exists(dst_dir):
        os.makedirs(dst_dir)
    open(dst, 'a').close()
    shutil.copy(src_path, dst_path)


def copy_to_dir_name(directory: str) -> str:
    return directory \
        .replace("build_output/", "", 1) \
        .replace(current_dir, result_dir)


subdir = sys.argv[1].replace("/.", "")
specific_elfs = [] if len(sys.argv) < 3 else sys.argv[2:]
print("Specific elf: ", specific_elfs)

current_dir = os.getcwd()
print("Current dir: ", current_dir)
result_dir = current_dir + "/" + "build_output"

subdir_path = current_dir + "/" + subdir
elfs = list_elfs_recursively(subdir_path)
print("All elfs: ", elfs)
filtered_elfs = filter_elfs(elfs, specific_elfs)
print("Filetered: ", filtered_elfs)
copy_args = [(elf, copy_to_dir_name(elf)) for elf in filtered_elfs]

for (src, dst) in copy_args:
    print(f"Cloning: {src}, to: {dst}")
    copy_file(src, dst)
