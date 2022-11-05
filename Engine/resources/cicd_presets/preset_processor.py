# -*- coding: utf-8 -*-
import hashlib
import shutil
from datetime import datetime


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

resource_version_locations = [
    "../../../GUI/resources/resource_versions.txt",
    "../../../GUI/resources/updatertarget/resource_versions.txt",
]


def main():
    current_hash = md5(current_files)
    new_hash = md5(files_to_process)
    if new_hash != current_hash:
        for pfile, dfiles in zip(files_to_process, target_copy_locations):
            for dfile in dfiles:
                shutil.copyfile(pfile, dfile)
        update_resource_versions()
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


def update_resource_versions():
    with open(resource_version_locations[0], "r") as orig_file:
        orig_file_lines = orig_file.readlines()
    version = orig_file_lines[0].split(";")[1]
    # current_date_string = datetime.today().strftime("%Y-%m-%d-%H-%M-%S")
    current_date_string = datetime.today().strftime("%Y-%m-%d")
    with open("./resource_versions.txt", "w") as new_file:
        for line in orig_file_lines:
            if line.startswith("presets"):
                new_file.write(f"presets;{version};{current_date_string}\n")
            elif line.startswith("nodeidorders"):
                new_file.write(f"nodeidorders;{version};{current_date_string}\n")
            else:
                new_file.write(line)
    for dfile in resource_version_locations:
        shutil.copyfile("./resource_versions.txt", dfile)


if __name__ == "__main__":
    print(main())
