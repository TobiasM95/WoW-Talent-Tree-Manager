# -*- coding: utf-8 -*-
import sys
import os
from dataclasses import dataclass
import re
import json
import asyncio
import aiohttp
import requests
import pickle
from timeit import default_timer as timer
import hashlib
import numpy as np

from PIL import Image
import concurrent.futures

TYPE_ACTIVE = "active"
TYPE_PASSIVE = "passive"
TYPE_CHOICE = "choice"
TYPES = [TYPE_ACTIVE, TYPE_PASSIVE, TYPE_CHOICE]


@dataclass
class Talent:
    index: int
    node_id: int
    definition_id: int
    names: list
    descriptions: list
    talent_type: str
    row: int
    column: int
    max_points: int
    required_points: int
    prefilled: bool
    parent_indices: list
    child_indices: list
    spell_ids: list
    tooltip_urls: list
    icon_urls: list
    icon_names: list
    original_index: int

    def __init__(self):
        self.index = -1
        self.node_id = -1
        self.definition_id = -1
        self.names = []
        self.descriptions = []
        self.talent_type = ""
        self.row = -1
        self.column = -1
        self.max_points = -1
        self.required_points = -1
        self.prefilled = False
        self.parent_indices = []
        self.child_indices = []
        self.icon_names = []

        self.spell_ids = []
        self.tooltip_urls = []
        self.icon_urls = []
        self.original_index = -1


if len(sys.argv) >= 2 and (
    sys.argv[1].lower() == "true" or sys.argv[1].lower() == "overwrite"
):
    overwrite_icons = True
else:
    overwrite_icons = False
icons_dir = "./icons/"

# we always package all files together as a unit
files_to_process = ["./presets.txt", "./node_id_orders.txt"]
current_files = ["../presets.txt", "../node_id_orders.txt"]
pack_name = "icons_packed.png"


def main():
    scrape_start = timer()
    print("Generate trees..")
    class_and_spec_trees, node_id_orders = generate_tree_structures_from_raidbots()

    tree_strings = []
    icon_download_info = []
    for class_name, class_trees_dict in class_and_spec_trees.items():
        for spec_name, tree_dict in class_trees_dict.items():
            tree_clean = clean_tree(tree_dict)
            tree_strings.append(create_tree_string(class_name, spec_name, tree_clean))
            for index, talent in tree_clean.items():
                for name, url in zip(talent.icon_names, talent.icon_urls):
                    icon_download_info.append([name, url])

    combine_tree_strings(tree_strings)
    export_node_orders_to_file(node_id_orders)

    current_hash = md5(current_files)
    new_hash = md5(files_to_process)
    print("current/old hash:", current_hash)
    print("new hash:", new_hash)
    icon_list = []
    if new_hash != current_hash and False:
        print(f"Download {len(icon_download_info)} icons..")
        with concurrent.futures.ThreadPoolExecutor(max_workers=32) as executor:
            future_to_url = {
                executor.submit(download_icon, name, url, icon_list): url
                for name, url in icon_download_info
            }
            for future in concurrent.futures.as_completed(future_to_url):
                url = future_to_url[future]
                try:
                    future.result()
                except Exception as exc:
                    print("%r generated an exception: %s" % (url, exc))
        pack_and_save_icons(icon_list)

    finish = timer()
    # print(f"Scraping took {download_start - scrape_start} seconds.")
    # print(f"Downloading took {finish - download_start} seconds.")
    # print(f"Everything took {finish - scrape_start} seconds.")
    print(f"Scraping took {finish - scrape_start} seconds.")


def generate_tree_structures_from_raidbots():
    class_and_spec_trees = {}
    node_id_orders = {}
    tree_info_json = requests.get(
        "https://www.raidbots.com/static/data/live/talents.json"
    ).json()

    for spec_dict in tree_info_json:
        class_name = spec_dict["className"].lower().replace(" ", "")
        spec_name = spec_dict["specName"].lower().replace(" ", "")

        if class_name not in class_and_spec_trees:
            class_and_spec_trees[class_name] = {}
        if class_name not in node_id_orders:
            node_id_orders[class_name] = {}

        class_and_spec_trees[class_name][f"class_{spec_name}"] = extract_tree(
            spec_dict["classNodes"], True, class_name, spec_name
        )
        class_and_spec_trees[class_name][spec_name] = extract_tree(
            spec_dict["specNodes"], False, class_name, spec_name
        )
        node_id_orders[class_name][spec_name] = (
            spec_dict["classId"],
            spec_dict["specId"],
            spec_dict["fullNodeOrder"],
        )

    class_and_spec_trees = fill_in_descriptions(class_and_spec_trees)

    return class_and_spec_trees, node_id_orders


def extract_tree(talents, is_class_tree, class_name, spec_name):
    tree_dict = {}
    node_id_index_map = {}
    x_offset = 1200 if is_class_tree else 9000
    y_offset = 1200
    for index, talent_dict in enumerate(talents):
        node_id_index_map[talent_dict["id"]] = index
        talent = Talent()

        talent.index = index
        talent.node_id = talent_dict["id"]

        # talent.names = talent_dict["name"].split(" / ")
        talent.names = [
            entry["name"]
            for entry in sorted(talent_dict["entries"], key=lambda x: x["index"])
        ]

        if talent_dict["type"] == "choice":
            talent.talent_type = TYPE_CHOICE
        elif talent_dict["maxRanks"] > 1:
            talent.talent_type = TYPE_PASSIVE
        else:
            talent.talent_type = TYPE_ACTIVE

        talent.row = (talent_dict["posY"] - y_offset) // 300
        talent.column = (talent_dict["posX"] - x_offset) // 300

        talent.max_points = talent_dict["maxRanks"]

        talent.required_points = (
            talent_dict["reqPoints"] if "reqPoints" in talent_dict else 0
        )

        talent.prefilled = "freeNode" in talent_dict

        talent.parent_indices = talent_dict["prev"]
        talent.child_indices = talent_dict["next"]

        talent.spell_ids = [
            entry["spellId"]
            for entry in sorted(talent_dict["entries"], key=lambda x: x["index"])
        ]
        talent.definition_ids = [
            entry["definitionId"]
            for entry in sorted(talent_dict["entries"], key=lambda x: x["index"])
        ]

        talent.tooltip_urls = [
            "https://www.wowhead.com/spell=" + str(entry["spellId"])
            for entry in sorted(talent_dict["entries"], key=lambda x: x["index"])
        ]

        talent.icon_names = [
            format_name(x) + class_name.capitalize()[:1] + spec_name.capitalize()[:3]
            for x in talent.names
        ]
        talent.icon_urls = [
            "https://wow.zamimg.com/images/wow/icons/large/" + entry["icon"] + ".jpg"
            for entry in sorted(talent_dict["entries"], key=lambda x: x["index"])
        ]

        tree_dict[index] = talent

    # transform node id indices of parent/child indices to internal index
    for index, talent in tree_dict.items():
        talent.parent_indices = [
            node_id_index_map[idx] for idx in talent.parent_indices
        ]
        talent.child_indices = [node_id_index_map[idx] for idx in talent.child_indices]

    return tree_dict


def fill_in_descriptions(class_and_spec_trees):
    urls = get_tooltip_urls(class_and_spec_trees)
    tooltip_jsons = get_tooltip_jsons(urls)
    for class_name, class_trees_dict in class_and_spec_trees.items():
        for spec_name, tree_dict in class_trees_dict.items():
            for index, talent in tree_dict.items():
                if talent.talent_type != TYPE_CHOICE:
                    talent.descriptions = [
                        re.sub(
                            r"<[^>]*>",
                            "",
                            tooltip_jsons[(talent.spell_ids[0], rank)]["tooltip"]
                            .split('<div class="q">')[1]
                            .split("</div>")[0]
                            .replace("<br />", "\n"),
                        )
                        .strip()
                        .replace("\n ", "\n")
                        .replace("\n\n.", ".")
                        .replace("\n.", ".")
                        if '<div class="q">'
                        in tooltip_jsons[(talent.spell_ids[0], rank)]["tooltip"]
                        else "Description not available"
                        for rank in range(1, talent.max_points + 1)
                    ]
                else:
                    talent.descriptions = [
                        re.sub(
                            r"<[^>]*>",
                            "",
                            tooltip_jsons[(talent.spell_ids[i], 1)]["tooltip"]
                            .split('<div class="q">')[1]
                            .split("</div>")[0]
                            .replace("<br />", "\n"),
                        )
                        .strip()
                        .replace("\n ", "\n")
                        .replace("\n\n.", ".")
                        .replace("\n.", ".")
                        if '<div class="q">'
                        in tooltip_jsons[(talent.spell_ids[i], 1)]["tooltip"]
                        else "Description not available"
                        for i in range(len(talent.spell_ids))
                    ]

    return class_and_spec_trees


def get_tooltip_urls(class_and_spec_trees):
    urls = []
    for class_name, class_trees_dict in class_and_spec_trees.items():
        for spec_name, tree_dict in class_trees_dict.items():
            for index, talent in tree_dict.items():
                if talent.talent_type == TYPE_CHOICE:
                    for sid, definition in zip(talent.spell_ids, talent.definition_ids):
                        urls.append(
                            [
                                [sid, 1],
                                f"https://nether.wowhead.com/tooltip/spell/{sid}?def={definition}&rank=1&dataEnv=3",
                            ]
                        )
                else:
                    for rank in range(1, talent.max_points + 1):
                        sid = talent.spell_ids[0]
                        definition = talent.definition_ids[0]
                        urls.append(
                            [
                                [sid, rank],
                                f"https://nether.wowhead.com/tooltip/spell/{sid}?{definition}rank={rank}&dataEnv=3",
                            ]
                        )
    return urls


def get_tooltip_jsons(urls):
    if os.name == "nt":
        asyncio.set_event_loop_policy(asyncio.WindowsSelectorEventLoopPolicy())
    res = asyncio.run(request_tooltips(urls))
    tooltip_jsons = {}
    for resp in res:
        j = json.loads(resp[1])
        tooltip_jsons[tuple(resp[0])] = j
    return tooltip_jsons


async def get(url, session):
    try:
        async with session.get(url=url[1]) as response:
            resp = await response.read()
    except Exception as e:
        print("Unable to get url {} due to {}.".format(url, e.__class__))
    return [url[0], resp]


async def request_tooltips(urls):
    async with aiohttp.ClientSession() as session:
        ret = await asyncio.gather(*[get(url, session) for url in urls])
    return ret


def format_name(name):
    name = re.sub(r"[^A-Za-z0-9 ]+", "", name)
    parts = name.split(" ")
    fname = parts[0].lower()
    for p in parts[1:]:
        if p == "[NNF]":
            continue
        fname += p.capitalize()
    return fname


def clean_tree(tree):
    min_row = 10101
    min_col = 10101
    talent_list = []
    for index, talent in tree.items():
        if talent.row < min_row:
            min_row = talent.row
        if talent.column < min_col:
            min_col = talent.column
        talent_list.append(talent)
    row_diff = min_row - 1
    col_diff = min_col - 1
    talent_list = sorted(talent_list, key=lambda x: (x.row, x.column))
    new_tree = {}
    index_map = {}
    for index, talent in enumerate(talent_list):
        index_map[talent.index] = index
    for index, talent in enumerate(talent_list):
        talent.original_index = talent.index
        talent.index = index
        talent.row -= row_diff
        # talent.row = talent.row * 2 - 1
        talent.column -= col_diff
        talent.child_indices = [index_map[x] for x in talent.child_indices]
        talent.parent_indices = [index_map[x] for x in talent.parent_indices]
        new_tree[index] = talent
    return new_tree


def create_tree_string(class_name, spec_name, tree):
    tree_string = preambel(class_name, spec_name, len(tree))

    for talent_index, talent in tree.items():
        tree_string += str(talent.index) + ":"
        tree_string += ",".join([clean_ttm_name(x) for x in talent.names]) + ":"
        tree_string += (
            ",".join(
                [
                    clean_string(x)
                    for x in talent.descriptions
                    if "to unlock this talent" not in x.lower()
                ]
            )
            + ":"
        )
        tree_string += str(TYPES.index(talent.talent_type)) + ":"
        tree_string += str(talent.row) + ":"
        tree_string += str(talent.column) + ":"
        tree_string += str(talent.max_points) + ":"
        tree_string += str(talent.required_points) + ":"
        tree_string += "1:" if talent.prefilled else "0:"
        tree_string += ",".join([str(x) for x in talent.parent_indices]) + ":"
        tree_string += ",".join([str(x) for x in talent.child_indices]) + ":"
        if len(talent.icon_names) == 1:
            talent.icon_names += ["default"]
        tree_string += ",".join([x + ".png" for x in talent.icon_names]) + ":"
        tree_string += str(talent.node_id) + ";"

        if (
            len(talent.descriptions) < talent.max_points
            and talent.talent_type == TYPE_PASSIVE
        ):
            if len(talent.names) == 0:
                from IPython import embed

                embed()

    return tree_string


def export_tree_to_file(class_name, spec_name, tree):
    trees_dir = "./individual_trees/"
    os.makedirs(trees_dir, exist_ok=True)

    tree_string = create_tree_string(class_name, spec_name, tree)

    filename = tree_string.split(":")[1]

    pickle.dump(tree, open(trees_dir + filename + ".pkl", "wb"))

    with open(trees_dir + filename + ".txt", "w") as outfile:
        outfile.write(tree_string)


def clean_ttm_name(name):
    clean_name = ""
    for c in name:
        if (
            c
            not in "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 _/()',-"
        ):
            continue
        clean_name += c
    return clean_name


def clean_string(string):
    string = string.replace(":", "__cl__")
    string = string.replace("\n", "__n__")
    string = string.replace(",", "__cm__")
    string = string.replace(";", "__sc__")
    return (
        string.replace("\342\200\231", "'")
        .replace("\342\200\231", "'")
        .replace("\222", "'")
        .encode("utf-8")
        .replace(b"\xc3\xa2\xe2\x82\xac\xe2\x84\xa2", b"'")
        .decode("utf-8")
    )


def download_icon(name, url, icon_list):
    target_size = (40, 40)
    success = False
    while "_" in url:
        try:
            img = Image.open(requests.get(url, stream=True).raw)
            success = True
        except:
            url = "-".join(url.rsplit("_", 1))
        if success:
            break
    if not success:
        print(f"error getting icon: {name} at url {url}")
        return
    img = img.resize(target_size, Image.Resampling.LANCZOS)
    img.putalpha(255)
    icon_list.append([name, img])


def pack_and_save_icons(icon_list):
    target_resolution = 40
    target_width = target_resolution * target_resolution
    icon_list = [
        ["default", Image.open("../../../GUI/resources/icons/default.png")]
    ] + icon_list
    target_image_data = 255 * np.ones((len(icon_list), target_width, 4)).astype(
        np.uint8
    )
    meta_info = [
        target_resolution,
        target_resolution,
        len(icon_list),
        target_width,
        "default.png",
    ]

    for i, (img_name, image) in enumerate(icon_list):
        image.putalpha(255)

        # resize image to target resolution
        if not image.size == (target_resolution, target_resolution):
            image.resize(
                (target_resolution, target_resolution), Image.Resampling.LANCZOS
            )

        im_arr = np.array(image)
        target_image_data[i] = im_arr.reshape((-1, 4))

        if i > 0:
            meta_info.append(f"{img_name}.png")

    target_image = Image.fromarray(target_image_data)
    target_image.save(pack_name, format="png")

    with open(pack_name.split(".")[0] + "_meta.txt", "w") as metafile:
        for info in meta_info:
            metafile.write(str(info) + "\n")


def download_icons(tree):
    icons_dir = "./icons/"
    target_size = (40, 40)
    target_format = "png"
    os.makedirs(icons_dir, exist_ok=True)

    for _, talent in tree.items():
        for name, url in zip(talent.icon_names, talent.icon_urls):
            # check if icon exists
            if name + ".png" in next(os.walk(icons_dir))[2]:
                continue
            # if not, download, format, and save
            filename = name + ".png"
            # print(f"Fetch icon {i+1}/{len(icon_urls)}: {filename}")
            try:
                img = Image.open(requests.get(url, stream=True).raw)
            except:
                print(f"error getting icon: {name} at url {url}")
                continue
            img = img.resize(target_size, Image.Resampling.LANCZOS)
            img.putalpha(255)
            img.save(icons_dir + filename, format=target_format)


def preambel(class_name, spec_name, num_talents, version="1.3.8"):
    is_spec_tree = 0 if "class_" in spec_name else 1
    id_name = f"{class_name}_{spec_name}"
    if is_spec_tree == 1:
        display_name = f"{spec_name.capitalize()} {class_name.capitalize()}"
        descriptor = display_name
    else:
        spec_name = spec_name.split("class_")[-1]
        display_name = f"{class_name.capitalize()} class ({spec_name.capitalize()})"
        descriptor = f"{class_name.capitalize()} class tree as {spec_name.capitalize()}"

    return f"{version}:{id_name}:{is_spec_tree}:{display_name}:This is the preset for the {descriptor}.__n__You can start editing the tree/loadout now.::{num_talents}:0;"


def combine_tree_strings(tree_strings, version="1.3.8"):
    preset_file_path = "./presets.txt"

    with open(preset_file_path, "w") as outfile:
        outfile.write(
            f'{version}:custom:0:New custom Tree:Welcome to WoW Talent Tree Manager.__n__This is a solution to manage trees and their loadouts. A loadout is a collection of skillsets, __n__e.g. a skillset for Raid/M+/PvP etc. You can edit trees or work with spec presets, manage __n__various skillsets in your loadout as well as auto solve all possible skill combinations with a __n__given filter.__n____n__Edit the tree name/description here. To edit tree nodes select "Tree Editor" in the top right.__n__Press "Save/Load Trees" to load tree spec presets, manage your custom trees and import or__n__export trees (from/to Discord, etc.).__n__To start the tree editing process, go to "Tree Editor" -> "Create Node" and create your first__n__talent.__n____n__Select the "Talent Loadout Editor" in the top left to manage your loadout. You can __n__edit the loadout description there and create/import/export skillsets. Since skillsets are stored__n__inside the loadout which is part of the tree, you can save your skillsets by saving the tree__n__in the tree editor.__n____n__Lastly, select the "Talent Loadout Solver" to generate all possible combinations of talent__n__selections for all possible amounts of spendable talent points. Afterwards, you can filter__n__the results to include/exclude specific talents and load the results into your loadout.__n____n__Hint__cl__ This text can be edited!:Your loadout is a collection of different skillset that you can edit to suit various ingame __n__situations, e.g. a raid setup, an M+ setup or different PvP skillsets.__n__All skillsets are stored in the loadout which in turn is stored in the tree. So if you save your tree__n__you\'ll save your loadout as well! Additionally, you can import/export skillsets directly, to share__n__with friends or your favorite discord class experts.:0:0;\n'
        )
        for ts in tree_strings:
            outfile.write(ts + "\n")


def export_node_orders_to_file(node_id_orders):
    node_id_order_file_path = "./node_id_orders.txt"
    with open(node_id_order_file_path, "w") as outfile:
        for class_name, class_node_ids_order_dict in node_id_orders.items():
            for spec_name, (
                class_id,
                spec_id,
                spec_node_ids_order,
            ) in class_node_ids_order_dict.items():
                outfile.write(
                    f"{class_name}_{spec_name}:{class_id}:{spec_id}:"
                    + ",".join([str(idx) for idx in spec_node_ids_order])
                    + "\n"
                )


def md5(filepaths):
    hash_md5 = hashlib.md5()
    for filepath in filepaths:
        with open(filepath, "rb") as f:
            for chunk in iter(lambda: f.read(4096), b""):
                hash_md5.update(chunk)
    return hash_md5.hexdigest()


if __name__ == "__main__":
    main()
