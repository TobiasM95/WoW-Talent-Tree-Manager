from components.create_app import app, db_handler
import json
from typing import Union
from database_handler import Workspace, Tree, Loadout, Build, Talent
from flask import jsonify, Response, request
from flask_jwt_extended import jwt_required, current_user


@app.route("/tree/<string:content_id>", methods=["GET", "POST"])
@jwt_required()
def tree(content_id: str) -> Response:
    if request.method == "GET":
        return jsonify(get_tree_and_talent_descriptions(content_id)), 200
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
    class_talents = [
        {
            key: json.loads(value) if key in ["child_ids", "parent_ids"] else value
            for key, value in vars(talent).items()
            if key != "content_id"
        }
        for talent in talents[: len(class_talent_ids)]
    ]
    spec_talents = [
        {key: value for key, value in vars(talent).items() if key != "content_id"}
        for talent in talents[: len(spec_talent_ids)]
    ]
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
