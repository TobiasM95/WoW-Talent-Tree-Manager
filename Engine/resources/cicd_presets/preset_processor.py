# -*- coding: utf-8 -*-
import sys
import hashlib
import shutil


# we always package all files together as a unit
files_to_process = ["./presets.txt", "./node_id_orders.txt"]

current_files = ["../presets.txt", "../node_id_orders.txt"]

target_copy_locations = [
    ["../presets.txt", "../../../GUI/resources/updatertarget/presets.txt"],
    [
        "../node_id_orders.txt",
        "../../../GUI/resources/updatertarget/node_id_orders.txt",
    ],
]


def main():
    current_hash = md5(current_files)
    new_hash = md5(files_to_process)
    if new_hash != current_hash:
        for pfile, dfiles in zip(files_to_process, target_copy_locations):
            for dfile in dfiles:
                # print(pfile, dfile)
                # shutil.copyfile(pfile, dfiles)
                pass
        return 1
    else:
        return 0


def md5(filepaths):
    hash_md5 = hashlib.md5()
    for filepath in filepaths:
        with open(filepath, "rb") as f:
            for chunk in iter(lambda: f.read(4096), b""):
                hash_md5.update(chunk)
    return hash_md5.hexdigest()


if __name__ == "__main__":
    print(main())
