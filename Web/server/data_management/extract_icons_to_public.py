import os
from tqdm import tqdm
from PIL import Image
import numpy as np


def extract_icons() -> None:
    SOURCE_DIR_PATH: str = os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        "..",
        "..",
        "..",
        "GUI",
        "resources",
        "updatertarget",
    )

    DESTINATION_DIR_PATH: str = os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        "..",
        "..",
        "frontend",
        "public",
        "preset_icons",
    )

    with open(os.path.join(SOURCE_DIR_PATH, "icons_packed_meta.txt"), "r") as meta_file:
        meta_data: list[str] = [l.strip() for l in meta_file.readlines()]
    image_width: int = int(meta_data[0])
    image_height: int = int(meta_data[1])
    num_images: int = int(meta_data[2])
    image_names: list[str] = meta_data[4:]
    assert (
        len(image_names) == num_images
    ), "Mismatch between len(image_names) and num_images"
    im: Image.Image = Image.open(os.path.join(SOURCE_DIR_PATH, "icons_packed.png"))
    im_arr: np.array = np.array(im)
    for i, image_name in enumerate(tqdm(image_names)):
        icon: Image.Image = Image.fromarray(
            im_arr[i, :, :].reshape((image_height, image_width, 4))
        )
        icon.save(os.path.join(DESTINATION_DIR_PATH, image_name), format="png")


if __name__ == "__main__":
    extract_icons()
