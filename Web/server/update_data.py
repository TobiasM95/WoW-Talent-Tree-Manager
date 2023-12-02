# NOTE:
# This file starts all necessary data update tasks if necessary and should be run daily
import os
import hashlib

from data_management.create_popular_builds import create_popular_builds
from data_management.preset_updater import update_presets
from data_management.extract_icons_to_public import extract_icons


def update_data():
    # Check if presets have been updated, if yes:
    if data_files_were_updated():
        print("Native TTM files were updated, updated web assets..")
        print("Update presets..")
        update_presets()
        print("Extract icons..")
        extract_icons()
    else:
        print("No native TTM file updates detected..")

    print("Create popular builds..")
    create_popular_builds()


def data_files_were_updated():
    hash_path = os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "data_management", "hash.txt"
    )

    presets_path = os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        "..",
        "..",
        "Engine",
        "resources",
        "cicd_presets",
        "presets.txt",
    )

    icon_meta_path: str = os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        "..",
        "..",
        "GUI",
        "resources",
        "updatertarget",
        "icons_packed_meta.txt",
    )
    hash_s256 = hashlib.sha256()
    with open(presets_path, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hash_s256.update(chunk)
    with open(icon_meta_path, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hash_s256.update(chunk)
    file_hash = hash_s256.hexdigest()

    if not os.path.isfile(hash_path):
        with open(hash_path, "w") as hash_file:
            hash_file.write(file_hash)
        return True

    with open(hash_path, "r") as hash_file:
        old_hash = hash_file.readline()

    if old_hash != file_hash:
        with open(hash_path, "w") as hash_file:
            hash_file.write(file_hash)
        return True

    return False


if __name__ == "__main__":
    update_data()
