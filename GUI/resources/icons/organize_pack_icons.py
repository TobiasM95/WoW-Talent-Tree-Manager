# -*- coding: utf-8 -*-
import sys
import os

from collections import defaultdict

import numpy as np
from PIL import Image


pack_name = "icons_packed.png"


def main():
    organize_icons()
    pack_icons()

def organize_icons():
    max_size = 100
    
    directory_sizes = {}
    icon_path_map = defaultdict(list)
    icon_dirs = next(os.walk("."))[1]
    for directory in icon_dirs:
        icon_names = next(os.walk(f"./{directory}/"))[2]
        directory_sizes["./" + directory + "/"] = len(icon_names)
        for name in icon_names:
            icon_path_map[name] = f"./{directory}/{name}"
    
    current_directory = None
    current_size = None
    last_directory = None
    for directory, size in directory_sizes.items():
        last_directory = directory
        if size < 100:
            current_directory = directory
            current_size = size
    if current_directory is None:
        if last_directory is None:
            current_directory = "./0/"
            current_size = 0
        else:
            current_directory = "./" + str(int(last_directory)) + "/"
            current_size = 0
            
    overwritten_icons = []
    image_names = [x for x in next(os.walk("."))[2] if x[-4:] == ".png" and x != "default.png" and x != pack_name]
    for image_name in image_names:
        if image_name in icon_path_map:
            overwritten_icons.append(icon_path_map[image_name])
            #move and overwrite existing image at path
            os.remove(icon_path_map[image_name])
            os.rename(f"./{image_name}", icon_path_map[image_name])
        else:
            #move image to current or new dir
            if current_size < 100:
                os.makedirs(current_directory, exist_ok=True)
                os.rename(f"./{image_name}", current_directory + image_name)
            else:
                current_directory = "./" + str(int(current_directory)) + "/"
                current_size = 0
                os.makedirs(current_directory, exist_ok=True)
                os.rename(f"./{image_name}", current_directory + image_name)

    print("Written files:", len(image_names))
    print("Including overwritten files:", len(overwritten_icons))
    if len(overwritten_icons) > 0:
        print("Overwritten icons:")
        for ow_icon in overwritten_icons:
            print(ow_icon)
        

def pack_icons():

    target_resolution = 40

    
    target_width = target_resolution*target_resolution
    images = ["./default.png"]
    for directory in next(os.walk("."))[1]:
        images += [f"./{directory}/" + x for x in next(os.walk(f"./{directory}"))[2] if x[-4:] == ".png" and x != pack_name]
    target_image_data = 255*np.ones((len(images), target_width, 4)).astype(np.uint8)
    meta_info = [
        target_resolution,
        target_resolution,
        len(images),
        target_width
    ]
    for i, image in enumerate(images):
        im = Image.open(image)
        im.putalpha(255)
        
        #resize image to target resolution
        if not im.size == (target_resolution, target_resolution):
            im.resize((target_resolution, target_resolution), Image.Resampling.LANCZOS)
        
        im_arr = np.array(im)
        target_image_data[i] = im_arr.reshape((-1,4))

        meta_info.append("/".join(image.split("/")[1:]))
    target_image = Image.fromarray(target_image_data)
    target_image.save(pack_name, format="png")

    with open(pack_name.split(".")[0] + "_meta.txt", "w") as metafile:
        for info in meta_info:
            metafile.write(str(info) + "\n")

if __name__ == "__main__":
    sys.exit(int(main() or 0))
