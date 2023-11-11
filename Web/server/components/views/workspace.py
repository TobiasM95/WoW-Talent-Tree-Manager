from components.create_app import app, db_handler
from typing import Union
from database_handler import Workspace, Tree, Loadout, Build
from flask import jsonify
from flask_jwt_extended import jwt_required, current_user
from .tree import find_root_tree, find_root_loadout, find_root_build


@app.route("/workspace", methods=["GET"])
@jwt_required()
def get_workspace():
    # first we get the raw workspace and required data
    workspace: Workspace = db_handler.get_workspace(current_user.user_id)
    # then we get the tree, loadout, build names
    resources: dict[str, dict[str, list]] = {"TREE": {}, "LOADOUT": {}, "BUILD": {}}
    for _, content_type, content_id, is_public in workspace.items:
        if content_type == "TREE":
            is_imported, root_tree = find_root_tree(content_id)
            resources[content_type][content_id] = [
                is_imported,
                root_tree.name,
                is_public,
            ]
        elif content_type == "LOADOUT":
            is_imported, root_loadout = find_root_loadout(content_id)
            _, root_tree = find_root_tree(root_loadout.tree_id)
            resources[content_type][content_id] = [
                is_imported,
                root_loadout.name,
                root_tree.name,
                is_public,
            ]
        else:  # BUILD
            (is_imported, root_build, leaf_loadout_id) = find_root_build(content_id)
            if root_build is not None:
                _, root_loadout = find_root_loadout(leaf_loadout_id)
            else:
                root_loadout = None
            _, root_tree = find_root_tree(root_build.tree_id)
            resources[content_type][content_id] = [
                is_imported,
                root_build.name,
                root_loadout.name if root_loadout is not None else None,
                root_tree.name,
                is_public,
            ]
    tree_column_definition: list[dict] = [
        {"field": "name", "headerName": "Name", "flex": 1},
        {"field": "isImported", "headerName": "Import", "convertCheckbox": True},
        {"field": "isPublic", "headerName": "Public", "convertCheckbox": True},
        {"field": "actions", "headerName": "Actions", "convertActions": True},
    ]
    loadout_column_definition: list[dict] = [
        {"field": "name", "headerName": "Name", "flex": 1},
        {"field": "isImported", "headerName": "Import", "convertCheckbox": True},
        {"field": "isPublic", "headerName": "Public", "convertCheckbox": True},
        {"field": "treeName", "headerName": "Tree name", "flex": 1},
        {"field": "actions", "headerName": "Actions", "convertActions": True},
    ]
    build_column_definition: list[dict] = [
        {"field": "name", "headerName": "Name", "flex": 1},
        {"field": "isImported", "headerName": "Import", "convertCheckbox": True},
        {"field": "isPublic", "headerName": "Public", "convertCheckbox": True},
        {"field": "treeName", "headerName": "Tree name", "flex": 1},
        {"field": "loadoutName", "headerName": "Loadout name", "flex": 1},
        {"field": "actions", "headerName": "Actions", "convertActions": True},
    ]
    tree_data: list[dict] = [
        {
            "name": info[1],
            "isImported": info[0],
            "isPublic": info[2],
            "actions": content_id,
        }
        for content_id, info in resources["TREE"].items()
    ]
    loadout_data: list[dict] = [
        {
            "name": info[1],
            "isImported": info[0],
            "treeName": info[2],
            "actions": content_id,
            "isPublic": info[3],
        }
        for content_id, info in resources["LOADOUT"].items()
    ]
    build_data: list[dict] = [
        {
            "name": info[1],
            "isImported": info[0],
            "treeName": info[3],
            "loadoutName": info[2],
            "actions": content_id,
            "isPublic": info[4],
        }
        for content_id, info in resources["BUILD"].items()
    ]
    response_data: dict = {
        "treeColumns": tree_column_definition,
        "loadoutColumns": loadout_column_definition,
        "buildColumns": build_column_definition,
        "treeData": tree_data,
        "loadoutData": loadout_data,
        "buildData": build_data,
    }

    return jsonify(response_data), 200
