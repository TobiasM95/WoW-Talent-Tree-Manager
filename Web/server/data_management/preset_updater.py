# NOTE:
# For now, this takes the original desktop app presets that already get
# updated automatically and converts them into the web version database format

import os
import uuid
from typing import Union
from sqlalchemy import create_engine
from database_handler import DBHandler, Tree, Talent
from pypika import Table, Query, Parameter
import json


def main() -> str:
    db_path: str = os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        "..",
        "database",
        "data.sqlite",
    )
    engine = create_engine(f"sqlite+pysqlite:///{db_path}", echo=False)
    db_handler: DBHandler = DBHandler(engine)

    presets: list[str] = load_presets()
    presets_converted: tuple[list[Tree], list[Talent]] = convert_presets(presets)
    update_db(db_handler, presets_converted)


def load_presets() -> list[str]:
    path = os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        "..",
        "..",
        "..",
        "Engine",
        "resources",
        "cicd_presets",
        "presets.txt",
    )
    with open(path, "r") as presets_file:
        presets: list[str] = presets_file.readlines()
    return presets


def convert_presets(presets: list[str]) -> tuple[list[Tree], list[Talent]]:
    trees: list[Tree] = []
    talents: list[Talent] = []

    combinations: list[list[str]] = []

    for preset in presets:
        desc: str = preset.split(";")[0].split(":")[1]
        if desc == "custom":
            tree_parts: list[str] = preset.split(";")
            meta_information: list[str] = tree_parts[0].split(":")
            tree: Tree = Tree(
                str(uuid.uuid4()),
                None,
                restore_string(meta_information[3]),
                ". ".join(restore_string(meta_information[4]).split(".")[:2]),
                [],
                [],
            )
            trees.append(tree)
            continue
        if "_class_" not in desc:
            continue
        combinations.append([desc, desc.replace("_class", "")])

    for combination in combinations:
        for preset in presets:
            desc: str = preset.split(";")[0].split(":")[1]
            if desc == combination[0]:
                class_tree_str: str = preset
            elif desc == combination[1]:
                spec_tree_str: str = preset

        class_talents: list[str] = []
        spec_talents: list[str] = []

        # Class tree part
        for is_spec_tree, tree_str in enumerate([class_tree_str, spec_tree_str]):
            tree_parts: list[str] = tree_str.split(";")
            meta_information: list[str] = tree_parts[0].split(":")
            tree_talents: list[str] = tree_parts[1:]
            for tree_talent_str in tree_talents:
                if ":" not in tree_talent_str or len(tree_talent_str) < 10:
                    continue
                talent_information = tree_talent_str.split(":")
                talent_type: str = (
                    "SWITCH"
                    if talent_information[3] == "2"
                    else ("ACTIVE" if talent_information[3] == "0" else "PASSIVE")
                )
                if talent_type == "SWITCH":
                    names = talent_information[1].split(",")
                    descriptions = talent_information[2].split(",")
                    name = names[0]
                    name_switch = names[1]
                    description = descriptions[0]
                    description_switch = descriptions[1]
                else:
                    name = talent_information[1]
                    name_switch = None
                    description = talent_information[2]
                    description_switch = None
                icon_names = talent_information[11].split(",")
                icon_name = icon_names[0]
                icon_name_switch = icon_names[1]
                tree_talent: Talent = Talent(
                    str(uuid.uuid4()),
                    int(talent_information[0]),
                    int(talent_information[-1]),
                    talent_type,
                    restore_string(name),
                    restore_string(name_switch),
                    restore_talent_description(description, talent_type),
                    restore_string(description_switch),
                    int(talent_information[4]),
                    int(talent_information[5]),
                    int(talent_information[6]),
                    int(talent_information[7]),
                    int(talent_information[8]),
                    [int(x) for x in talent_information[9].split(",") if x != ""],
                    [int(x) for x in talent_information[10].split(",") if x != ""],
                    icon_name,
                    icon_name_switch,
                )
                talents.append(tree_talent)
                if is_spec_tree == 0:
                    class_talents.append(tree_talent.content_id)
                else:
                    spec_talents.append(tree_talent.content_id)

        tree: Tree = Tree(
            str(uuid.uuid4()),
            None,
            restore_string(meta_information[3]),
            restore_string(meta_information[4]).split(".")[0] + ".",
            class_talents=class_talents,
            spec_talents=spec_talents,
        )
        trees.append(tree)

    return (trees, talents)


def update_db(
    db_handler: DBHandler, presets_converted: tuple[list[Tree], list[Talent]]
) -> None:
    db_handler.execute_query(Query.from_("PresetTrees").delete())
    db_handler.execute_query(Query.from_("PresetTalents").delete())
    db_handler.create_trees(presets_converted[0], custom=False)
    db_handler.create_talents(presets_converted[1], custom=False)


def restore_talent_description(description: Union[str, None], talent_type: str):
    if description is None:
        return None
    if talent_type != "PASSIVE":
        return restore_string(description)
    descriptions = description.split(",")
    return json.dumps([restore_string(s) for s in descriptions])


def restore_string(orig: Union[str, None]) -> str:
    if orig is None:
        return None
    return (
        orig.replace("__cl__", ":")
        .replace("__n__", "\n")
        .replace("__cm__", ",")
        .replace("__sc__", ";")
    )


if __name__ == "__main__":
    main()
