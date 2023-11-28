from components.create_app import app, db_handler
import os
import json
from typing import Union
from database_handler import Workspace, Tree, Loadout, Build, Talent
from flask import jsonify, Response, request
from flask_jwt_extended import jwt_required, current_user


@app.route("/tree/<string:content_id>", methods=["GET", "POST"])
@jwt_required()
def tree(content_id: str) -> Response:
    if not db_handler.validate_content_access(current_user.user_id, content_id):
        return (
            jsonify(
                {"success": False, "msg": "User does not have access to this content"}
            ),
            200,
        )
    if request.method == "GET":
        return (
            jsonify(
                {"success": True, "msg": get_tree_and_talent_descriptions(content_id)}
            ),
            200,
        )
    else:
        return jsonify(content_id), 200


@app.route("/loadout/<string:content_id>", methods=["GET", "POST"])
@jwt_required()
def loadout(content_id: str) -> Response:
    if not db_handler.validate_content_access(current_user.user_id, content_id):
        return (
            jsonify(
                {"success": False, "msg": "User does not have access to this content"}
            ),
            200,
        )
    if request.method == "GET":
        return (
            jsonify(
                {
                    "success": True,
                    "msg": get_loadout_information(content_id, current_user.user_id),
                }
            ),
            200,
        )
    else:
        return jsonify(content_id), 200


@app.route("/build/<string:content_id>", methods=["GET", "POST"])
@jwt_required()
def build(content_id: str) -> Response:
    if not db_handler.validate_content_access(current_user.user_id, content_id):
        return (
            jsonify(
                {"success": False, "msg": "User does not have access to this content"}
            ),
            200,
        )
    if request.method == "GET":
        return jsonify({"success": True, "msg": get_build_information(content_id)}), 200
    else:
        return jsonify(content_id), 200


@app.route("/build/special/<string:class_name>/<string:spec_name>", methods=["GET"])
@jwt_required()
def special_builds(class_name: str, spec_name: str) -> Response:
    return (
        jsonify(
            {
                "success": True,
                "msg": get_special_builds_content_ids(class_name, spec_name),
            }
        ),
        200,
    )


@app.route("/tree/preset/<string:class_name>/<string:spec_name>")
@jwt_required()
def get_preset_tree_content_id(class_name: str, spec_name: str) -> Response:
    content_id: Union[str, None] = db_handler.get_preset_tree_content_id(
        class_name.capitalize(), spec_name.capitalize()
    )
    if content_id is None:
        return (
            jsonify(
                {
                    "success": False,
                    "msg": f"No preset tree content id for {class_name} {spec_name} found",
                }
            ),
            200,
        )
    return (
        jsonify(
            {
                "success": True,
                "msg": content_id,
            }
        ),
        200,
    )


def get_special_builds_content_ids(class_name: str, spec_name: str) -> dict:
    encounter_ids = json.loads(os.environ["WCL_ENCOUNTER_IDS"])
    id_to_index = {id_value: index for index, id_value in enumerate(encounter_ids)}
    encounter_names = json.loads(os.environ["WCL_ENCOUNTER_NAMES"])
    encounter_dict = {x[0]: x[1] for x in zip(encounter_ids, encounter_names)}
    (
        top_build_content_ids,
        outlier_build_content_ids,
    ) = db_handler.get_preset_builds_content_ids(
        class_name.capitalize(), spec_name.capitalize()
    )
    top_build_content_ids = sorted(
        top_build_content_ids, key=lambda x: id_to_index[x[0]]
    )
    outlier_build_content_ids = sorted(
        outlier_build_content_ids, key=lambda x: id_to_index[x[0]]
    )
    column_definitions: list[dict] = [
        {"field": "encounter", "headerName": "Encounter", "flex": 1},
        {"field": "actions", "headerName": "Best build", "convertActions": True},
        {
            "field": "secondActions",
            "headerName": "Weird build",
            "convertSecondActions": True,
        },
    ]
    special_build_data: list[dict] = [
        {
            "encounter": encounter_dict[top_info[0]],
            "actions": top_info[1],
            "secondActions": outlier_info[1],
        }
        for top_info, outlier_info in zip(
            top_build_content_ids, outlier_build_content_ids
        )
    ]
    return {"buildColumns": column_definitions, "buildData": special_build_data}


def get_tree_and_talent_descriptions(content_id: str) -> dict:
    is_imported, tree = find_root_tree(content_id)
    if tree is None:
        return None
    return format_tree_and_talent_description(is_imported, tree)


def find_root_tree(content_id: str) -> tuple[Union[bool, None], Union[Tree, None]]:
    is_imported: bool = False
    tree: Tree = db_handler.get_tree(content_id, custom=None)
    if tree is None:
        return None, None
    while tree.name is None:
        is_imported = True
        print(tree, tree.content_id, tree.name, tree.import_id)
        tree = db_handler.get_tree(tree.import_id, custom=None)
        if tree.import_id is None and tree.name is None:
            raise Exception(
                f"Import ID and name None at the same time {tree.content_id}"
            )

    return is_imported, tree


def format_tree_and_talent_description(is_imported: bool, tree: Tree) -> dict:
    res: dict = {
        "name": tree.name,
        "description": tree.description,
        "isImported": is_imported,
    }
    class_talent_ids: list[str] = json.loads(tree.class_talents)
    spec_talent_ids: list[str] = json.loads(tree.spec_talents)
    # load talent descriptions
    talents: list[Talent] = db_handler.get_talents(
        class_talent_ids + spec_talent_ids, None
    )
    class_talents = {
        talent.order_id: {
            key: json.loads(value) if key in ["child_ids", "parent_ids"] else value
            for key, value in vars(talent).items()
            if key != "content_id"
        }
        for talent in talents[: len(class_talent_ids)]
    }
    spec_talents = {
        talent.order_id: {
            key: json.loads(value) if key in ["child_ids", "parent_ids"] else value
            for key, value in vars(talent).items()
            if key != "content_id"
        }
        for talent in talents[len(class_talent_ids) :]
    }
    res["classTalents"] = class_talents
    res["specTalents"] = spec_talents

    return res


def get_loadout_information(content_id: str, user_id: str) -> dict:
    is_loadout_imported, loadout = find_root_loadout(content_id)
    if loadout is None:
        return None
    loadout_build_content_ids: list[str] = db_handler.get_loadout_builds(
        user_id, content_id
    )
    loadout_builds: dict[str, dict] = {}
    for bcid in loadout_build_content_ids:
        is_build_imported, build, _ = find_root_build(bcid)
        loadout_builds[bcid] = {"name": build.name, "isImported": is_build_imported}
    tree_info: dict = get_tree_and_talent_descriptions(loadout.tree_id)
    return format_loadout_information(
        is_loadout_imported, loadout, loadout_builds, tree_info
    )


def find_root_loadout(
    content_id: str,
) -> tuple[Union[bool, None], Union[Loadout, None]]:
    is_imported: bool = False
    loadout: Union[Loadout, None] = db_handler.get_loadout(content_id)
    if loadout is None:
        return None, None
    while loadout.name is None:
        is_imported = True
        loadout: Loadout = db_handler.get_loadout(loadout.import_id)
        if loadout.import_id is None and loadout.name is None:
            raise Exception(
                f"Import ID and name None at the same time {loadout.content_id}"
            )

    return is_imported, loadout


def format_loadout_information(
    is_loadout_imported: bool,
    loadout: Loadout,
    loadout_builds: list[dict[str, str]],
    tree_info: dict,
    include_talent_descriptions: bool,
) -> dict:
    loadout_info: dict = {
        "name": loadout.name,
        "isImported": is_loadout_imported,
        "description": loadout.description,
        "treeName": tree_info["name"],
        "builds": loadout_builds,
    }

    if include_talent_descriptions:
        loadout_info["classTalents"] = tree_info["classTalents"]
        loadout_info["specTalents"] = tree_info["specTalents"]

    return loadout_info


def get_build_information(content_id: str) -> dict:
    is_build_imported, build, leaf_loadout_id = find_root_build(content_id)
    if build is None:
        return None
    loadout_info: Union[dict, None] = (
        get_loadout_information(leaf_loadout_id)
        if leaf_loadout_id is not None
        else None
    )
    tree_info: dict = get_tree_and_talent_descriptions(build.tree_id)
    return format_build_information(is_build_imported, build, loadout_info, tree_info)


def find_root_build(
    content_id: str,
) -> tuple[Union[bool, None], Union[Build, None], Union[str, None]]:
    is_build_imported: bool = False
    build: Union[Build, None] = db_handler.get_build(content_id, custom=None)
    if build is None:
        return None, None, None
    leaf_loadout_id: Union[str, None] = build.loadout_id
    while build.name is None:
        is_build_imported = True
        build: Build = db_handler.get_build(build.import_id, custom=None)
        if build.import_id is None and build.name is None:
            raise Exception(
                f"Import ID and name None at the same time {build.content_id}"
            )

    return is_build_imported, build, leaf_loadout_id


def format_build_information(
    is_build_imported: bool,
    build: Build,
    early_loadout: Union[Loadout, None],
    tree_info: dict,
    include_talent_descriptions: bool = True,
) -> dict:
    build_info: dict[str, Union[int, str, dict]] = {
        "name": build.name,
        "isImported": is_build_imported,
        "levelCap": build.level_cap,
        "useLevelCap": build.use_level_cap,
        "assignedSkills": json.loads(build.assigned_skills),
        "description": build.description,
        "treeName": tree_info["name"],
        "loadoutName": "No loadout" if early_loadout is None else early_loadout.name,
    }
    if include_talent_descriptions:
        build_info["classTalents"] = tree_info["classTalents"]
        build_info["specTalents"] = tree_info["specTalents"]
    return build_info
