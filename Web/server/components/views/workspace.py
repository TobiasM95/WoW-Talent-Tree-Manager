from components.create_app import app, db_handler
from typing import Union
from database_handler import Workspace, Tree, Loadout, Build
from flask import jsonify
from flask_jwt_extended import jwt_required, current_user
from .tree import find_root_tree


@app.route("/workspace", methods=["GET"])
@jwt_required()
def get_workspace():
    # first we get the raw workspace and required data
    workspace: Workspace = db_handler.get_workspace(current_user.user_id)
    # then we get the tree, loadout, build names
    resources: dict[str, dict[str, list]] = {"TREE": {}, "LOADOUT": {}, "BUILD": {}}
    for _, content_type, content_id in workspace.items:
        if content_type == "TREE":
            is_imported, root_tree = find_root_tree(content_id)
            resources[content_type][content_id] = [is_imported, root_tree.name]
        elif content_type == "LOADOUT":
            is_imported, root_loadout = find_root_loadout(content_id)
            _, root_tree = find_root_tree(root_loadout.tree_id)
            resources[content_type][content_id] = [
                is_imported,
                root_loadout.name,
                root_tree.name,
            ]
        else:  # BUILD
            is_imported, root_build, early_loadout = find_root_build(content_id)
            if early_loadout is not None:
                _, root_loadout = find_root_loadout(early_loadout.content_id)
            else:
                root_loadout = None
            _, root_tree = find_root_tree(root_build.tree_id)
            resources[content_type][content_id] = [
                is_imported,
                root_build.name,
                root_loadout.name if root_loadout is not None else None,
                root_tree.name,
            ]
    tree_column_definition: list[dict] = [
        {"field": "name", "headerName": "Name", "flex": 1},
        {"field": "isImported", "headerName": "Import", "convertCheckbox": True},
        {"field": "actions", "headerName": "Actions", "convertActions": True},
    ]
    loadout_column_definition: list[dict] = [
        {"field": "name", "headerName": "Name", "flex": 1},
        {"field": "isImported", "headerName": "Import", "convertCheckbox": True},
        {"field": "treeName", "headerName": "Tree name", "flex": 1},
        {"field": "actions", "headerName": "Actions", "convertActions": True},
    ]
    build_column_definition: list[dict] = [
        {"field": "name", "headerName": "Name", "flex": 1},
        {"field": "isImported", "headerName": "Import", "convertCheckbox": True},
        {"field": "treeName", "headerName": "Tree name", "flex": 1},
        {"field": "loadoutName", "headerName": "Loadout name", "flex": 1},
        {"field": "actions", "headerName": "Actions", "convertActions": True},
    ]
    tree_data: list[dict] = [
        {"name": info[1], "isImported": info[0], "actions": content_id}
        for content_id, info in resources["TREE"].items()
    ]
    loadout_data: list[dict] = [
        {
            "name": info[1],
            "isImported": info[0],
            "treeName": info[2],
            "actions": content_id,
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
