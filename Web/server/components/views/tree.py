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
        return [None, None]
    res: dict = {"name": tree.name, "description": tree.description}
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

    return [is_imported, res]


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


def get_build_information(content_id: str) -> dict:
    is_imported, build, early_loadout = find_root_build(content_id)
    build_info: dict[str, Union[int, str, dict]] = {
        "name": build.name,
        "levelCap": build.level_cap,
        "useLevelCap": build.use_level_cap,
        "assignedSkills": json.loads(build.assigned_skills),
    }
    tree_info: list[bool, dict] = get_tree_and_talent_descriptions(build.tree_id)
    return {
        "isImported": is_imported,
        "classTalents": tree_info[1]["classTalents"],
        "specTalents": tree_info[1]["specTalents"],
        "treeName": tree_info[1]["name"],
        "loadoutName": "No loadout" if early_loadout is None else early_loadout.name,
        "buildInformation": build_info,
    }


# Additionally, this returns the earliest loadout found in the import chain
def find_root_build(content_id: str) -> tuple[bool, Build, Union[Loadout, None]]:
    is_imported: bool = False
    build: Build = db_handler.get_build(content_id)
    early_loadout: Union[Loadout, None] = (
        find_root_loadout(build.loadout_id)[1] if build.loadout_id is not None else None
    )
    while build.name is None:
        is_imported = True
        build: Build = db_handler.get_build(build.import_id)
        if early_loadout is None:
            early_loadout: Union[Loadout, None] = (
                find_root_loadout(build.loadout_id)[1]
                if build.loadout_id is not None
                else None
            )
        if build.import_id is None and build.name is None:
            raise Exception(
                f"Import ID and name None at the same time {build.content_id}"
            )

    return is_imported, build, early_loadout


def find_root_loadout(content_id: str) -> tuple[bool, Loadout]:
    is_imported: bool = False
    loadout: Loadout = db_handler.get_loadout(content_id)
    while loadout.name is None:
        is_imported = True
        loadout: Loadout = db_handler.get_loadout(loadout.import_id)
        if loadout.import_id is None and loadout.name is None:
            raise Exception(
                f"Import ID and name None at the same time {loadout.content_id}"
            )

    return is_imported, loadout
