from components.create_app import app, db_handler
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
        return jsonify(get_tree_and_talent_descriptions(content_id)), 200
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
        return jsonify(get_loadout_information(content_id, current_user.user_id)), 200
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
        return jsonify(get_build_information(content_id)), 200
    else:
        return jsonify(content_id), 200


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
    build: Union[Build, None] = db_handler.get_build(content_id)
    if build is None:
        return None, None, None, None
    leaf_loadout_id: Union[str, None] = build.loadout_id
    while build.name is None:
        is_build_imported = True
        build: Build = db_handler.get_build(build.import_id)
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
        "treeName": tree_info["name"],
        "loadoutName": "No loadout" if early_loadout is None else early_loadout.name,
    }
    if include_talent_descriptions:
        build_info["classTalents"] = tree_info["classTalents"]
        build_info["specTalents"] = tree_info["specTalents"]
    return build_info
