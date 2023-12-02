# This viwe combines tree, loadout and build api to fetch all the components
# that connect to the content id, i.e. tree for a tree content id,
# loadut + tree for a loadout content id and build + loadout + tree for a
# build content id

import uuid
from components.create_app import app, db_handler
from typing import Union
from database_handler import Workspace, Tree, Loadout, Build
from flask import jsonify, Response
from flask_jwt_extended import jwt_required, current_user
from .tree import (
    find_root_tree,
    find_root_loadout,
    find_root_build,
    format_tree_and_talent_description,
    format_build_information,
    format_loadout_information,
)


@app.route("/content/<string:content_id>", methods=["GET"])
@jwt_required()
def content(content_id: str) -> Response:
    content_type, in_user_workspace, is_public = db_handler.validate_content_access(
        current_user.user_id, content_id
    )
    if content_type is None:
        return (
            jsonify(
                {"success": False, "msg": "User does not have access to this content"}
            ),
            200,
        )
    if content_type == "TREE":
        msg: dict = get_tree_content(content_id)
    elif content_type == "LOADOUT":
        msg: dict = get_loadout_content(content_id, current_user.user_id)
    elif content_type == "BUILD":
        msg: dict = get_build_content(content_id, current_user.user_id)
    else:
        return (
            jsonify(
                {
                    "success": False,
                    "msg": f"Content type {content_type} for content id {content_id} is not supported",
                }
            ),
            200,
        )

    msg["in_user_workspace"] = in_user_workspace
    return jsonify({"success": True, "msg": msg})


@app.route("/content/copyimport/<string:content_id>", methods=["POST"])
@jwt_required()
def copy_import(content_id: str) -> Response:
    content_type, in_user_workspace, is_public = db_handler.validate_content_access(
        current_user.user_id, content_id
    )
    if content_type is None:
        return (
            jsonify(
                {"success": False, "msg": "User does not have access to this content"}
            ),
            200,
        )

    new_content_id = str(uuid.uuid4())

    if content_type == "TREE":
        copy_import_tree(content_id, new_content_id, in_user_workspace)
    elif content_type == "LOADOUT":
        copy_import_loadout(content_id, new_content_id, in_user_workspace)
    elif content_type == "BUILD":
        copy_import_build(content_id, new_content_id, in_user_workspace)

    workspace: Workspace = Workspace(
        [[current_user.user_id, content_type, new_content_id, is_public]]
    )
    db_handler.create_workspace(workspace)

    return jsonify({"success": True, "msg": {"contentID": new_content_id}})


def get_build_content(content_id: str, user_id: str) -> dict:
    is_build_imported, root_build, leaf_loadout_id = find_root_build(content_id)
    is_loadout_imported, root_loadout = find_root_loadout(leaf_loadout_id)
    is_tree_imported, root_tree = find_root_tree(root_build.tree_id)
    res: dict = {"contentType": "BUILD"}
    res["tree"] = format_tree_and_talent_description(is_tree_imported, root_tree)
    res["build"] = format_build_information(
        is_build_imported,
        root_build,
        root_loadout,
        res["tree"],
        False,
    )
    if root_loadout is not None:
        loadout_build_content_ids: list[str] = db_handler.get_loadout_builds(
            user_id, leaf_loadout_id
        )
        loadout_builds: dict[str, dict] = {}
        loadout_builds["columns"] = [
            {"field": "name", "headerName": "Name", "flex": 1},
            {"field": "actions", "headerName": "Actions", "convertActions": True},
        ]
        loadout_builds["rows"] = []
        for bcid in loadout_build_content_ids:
            _, build, _ = find_root_build(bcid)
            loadout_builds["rows"].append({"name": build.name, "actions": bcid})
        res["loadout"] = format_loadout_information(
            is_loadout_imported, root_loadout, loadout_builds, res["tree"], False
        )
    else:
        res["loadout"] = None
    return res


def get_loadout_content(content_id: str, user_id: str) -> dict:
    is_loadout_imported, root_loadout = find_root_loadout(content_id)
    is_tree_imported, root_tree = find_root_tree(root_loadout.tree_id)
    res: dict = {"contentType": "LOADOUT"}
    res["tree"] = format_tree_and_talent_description(is_tree_imported, root_tree)
    loadout_build_content_ids: list[str] = db_handler.get_loadout_builds(
        user_id, content_id
    )
    loadout_builds: dict[str, dict] = {}
    loadout_builds["columns"] = [
        {"field": "name", "headerName": "Name", "flex": 1},
        {"field": "actions", "headerName": "Actions", "convertActions": True},
    ]
    loadout_builds["rows"] = []
    for bcid in loadout_build_content_ids:
        _, build, _ = find_root_build(bcid)
        loadout_builds["rows"].append({"name": build.name, "actions": bcid})
    res["loadout"] = format_loadout_information(
        is_loadout_imported, root_loadout, loadout_builds, res["tree"], False
    )
    return res


def get_tree_content(content_id: str) -> dict:
    is_imported, tree = find_root_tree(content_id)
    res: dict = {"contentType": "TREE"}
    res["tree"] = format_tree_and_talent_description(is_imported, tree)
    return res


def copy_import_tree(content_id: str, new_content_id: str, make_copy: bool) -> None:
    if make_copy:
        tree: Tree = db_handler.get_tree(content_id, True)
        tree.content_id = new_content_id
        db_handler.create_tree(tree, True)
    else:  # make import
        db_handler.import_tree(new_content_id, content_id)


def copy_import_loadout(content_id: str, new_content_id: str, make_copy: bool) -> None:
    if make_copy:
        loadout: Loadout = db_handler.get_loadout(content_id)
        loadout.content_id = new_content_id
        db_handler.create_loadout(loadout)
    else:  # make import
        db_handler.import_loadout(new_content_id, content_id)


def copy_import_build(content_id: str, new_content_id: str, make_copy: bool) -> None:
    if make_copy:
        build: Build = db_handler.get_build(content_id, True)
        build.content_id = new_content_id
        build.loadout_id = None
        db_handler.create_build(build)
    else:  # make import
        db_handler.import_build(new_content_id, content_id)
