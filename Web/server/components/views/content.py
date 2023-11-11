# This viwe combines tree, loadout and build api to fetch all the components
# that connect to the content id, i.e. tree for a tree content id,
# loadut + tree for a loadout content id and build + loadout + tree for a
# build content id

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
    content_type: Union[str, None] = db_handler.validate_content_access(
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
        return get_tree_content(content_id)
    elif content_type == "LOADOUT":
        return get_loadout_content(content_id, current_user.user_id)
    elif content_type == "BUILD":
        return get_build_content(content_id, current_user.user_id)

    return (
        jsonify(
            {
                "success": False,
                "msg": f"Content type {content_type} for content id {content_id} is not supported",
            }
        ),
        200,
    )


def get_build_content(content_id: str, user_id: str) -> Response:
    is_build_imported, root_build, leaf_loadout_id = find_root_build(content_id)
    is_loadout_imported, root_loadout = find_root_loadout(leaf_loadout_id)
    is_tree_imported, root_tree = find_root_tree(root_build.tree_id)
    res: dict = {}
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
        for bcid in loadout_build_content_ids:
            is_build_imported, build, _ = find_root_build(bcid)
            loadout_builds[bcid] = {"name": build.name, "isImported": is_build_imported}
        res["loadout"] = format_loadout_information(
            is_loadout_imported, root_loadout, loadout_builds, res["tree"], False
        )
    else:
        res["loadout"] = None
    return jsonify({"success": True, "msg": res})


def get_loadout_content(content_id: str, user_id: str) -> Response:
    pass


def get_tree_content(content_id: str) -> Response:
    is_imported, tree = find_root_tree(content_id)
    return jsonify({"success": True, "msg": f"{is_imported},"})
