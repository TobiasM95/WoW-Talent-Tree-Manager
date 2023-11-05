import os
import json
from typing import Literal, Union, Sequence, Any
import datetime
from pypika import Table, Query, Parameter
from sqlalchemy import Engine, text, TextClause, Row


class Login:
    def __init__(
        self,
        user_id: str,
        alt_user_id: str,
        auth_method: Literal["PW", "SSO"],
        email: str,
        password_hash: bytes,
        salt: bytes,
        user_name: str,
        last_login_timestamp: datetime.datetime,
        is_activated: bool,
    ):
        self.user_id: str = user_id
        self.alt_user_id: str = alt_user_id
        self.auth_method: Literal["PW", "SSO"] = auth_method
        self.email: str = email
        self.password_hash: bytes = password_hash
        self.salt: bytes = salt
        self.user_name: str = user_name
        self.last_login_timestamp: datetime.datetime = last_login_timestamp
        self.is_activated: bool = is_activated


class Activation:
    def __init__(self, alt_user_id: str, activation_id: str):
        self.alt_user_id: str = alt_user_id
        self.activation_id: str = activation_id


class Workspace:
    def __init__(
        self, items: list[tuple[str, Literal["TREE", "LOADOUT", "BUILD"], str]]
    ):
        self.items = items if items is not None else []


class Tree:
    def __init__(
        self,
        content_id: str,
        import_id: Union[str, None],
        name: str,
        description: str,
        class_talents: Union[str, list[str]],
        spec_talents: Union[str, list[str]],
    ):
        self.content_id: str = content_id
        self.import_id: Union[str, None] = import_id
        self.name: str = name
        self.description: str = description
        if type(class_talents) == str:
            self.class_talents = class_talents
        else:
            self.class_talents: str = json.dumps(class_talents)
        if type(spec_talents) == str:
            self.spec_talents = spec_talents
        else:
            self.spec_talents: str = json.dumps(spec_talents)


class Loadout:
    def __init__(
        self, content_id: str, import_id: str, tree_id: str, name: str, description: str
    ):
        self.content_id: str = content_id
        self.import_id: str = import_id
        self.tree_id: str = tree_id
        self.name: str = name
        self.description: str = description


class Build:
    def __init__(
        self,
        content_id: str,
        import_id: str,
        tree_id: str,
        loadout_id: str,
        name: str,
        level_cap: int,
        use_level_cap: bool,
        assigned_skills: Union[str, dict[int, int]],
    ):
        self.content_id: str = content_id
        self.import_id: str = import_id
        self.tree_id: str = tree_id
        self.loadout_id: str = loadout_id
        self.name: str = name
        self.level_cap: int = level_cap
        self.use_level_cap: bool = use_level_cap
        if type(assigned_skills) == str:
            self.assigned_skills = assigned_skills
        else:
            self.assigned_skills: str = json.dumps(assigned_skills)


class Talent:
    def __init__(
        self,
        content_id: str,
        order_id: int,
        node_id: int,
        t_type: str,
        name: str,
        name_switch: str,
        description: Union[str, list[str]],
        description_switch: str,
        row: int,
        column: int,
        max_points: int,
        required_points: int,
        pre_filled: bool,
        parent_ids: Union[str, list[int]],
        child_ids: Union[str, list[int]],
        icon_name: str,
        icon_name_switch: str,
    ):
        self.content_id: str = content_id
        self.order_id: int = order_id
        self.node_id: int = node_id
        self.talent_type: str = t_type
        self.name: str = name
        self.name_switch: str = name_switch
        if type(description) == str:
            self.description: str = description
        else:
            self.description: str = json.dumps(description)
        self.description_switch: str = description_switch
        self.row: int = row
        self.column: int = column
        self.max_points: int = max_points
        self.required_points: int = required_points
        self.pre_filled: bool = pre_filled
        if type(parent_ids) == str:
            self.parent_ids: str = parent_ids
        else:
            self.parent_ids: str = json.dumps(parent_ids)
        if type(child_ids) == str:
            self.child_ids: str = child_ids
        else:
            self.child_ids: str = json.dumps(child_ids)
        self.icon_name: str = icon_name
        self.icon_name_switch: str = icon_name_switch


class Like:
    def __init__(self, user_id: str, content_id: str, timestamp: datetime.datetime):
        self.user_id: str = user_id
        self.content_id: str = content_id
        self.timestamp: datetime.datetime = timestamp


class Comment:
    def __init__(
        self,
        comment_id: str,
        user_id: str,
        content_id: str,
        message: str,
        timestamp: datetime.datetime,
        reply_id: Union[None, str],
        was_edited: bool,
    ):
        self.comment_id: str = comment_id
        self.user_id: str = user_id
        self.content_id: str = content_id
        self.message: str = message
        self.timestamp: datetime.datetime = timestamp
        self.reply_id: Union[None, str] = reply_id
        self.was_edited: bool = was_edited


class Feedback:
    def __init__(self, feedback_id: str, user_id: str, message: str):
        self.feedback_id: str = feedback_id
        self.user_id: str = user_id
        self.message: str = message


class DBHandler:
    def __init__(self, engine: Engine) -> None:
        self.engine: Engine = engine

    def initialize_databases(self, reset: bool = False) -> None:
        with self.engine.connect() as conn:
            is_initialized: bool = (
                conn.execute(
                    text("SELECT count(*) FROM sqlite_master WHERE type='table'")
                ).first()[0]
                == 8
            )
            conn.commit()

        if is_initialized and not reset:
            print("Database already initialized! Skip..")
            return

        if reset:
            self.engine.dispose()
            db_path = os.path.join(
                os.path.dirname(os.path.abspath(__file__)),
                "database",
                "data.sqlite",
            )
            if os.path.isfile(db_path):
                os.remove(db_path)

        with self.engine.connect() as conn:
            # fmt: off
            conn.execute(text("CREATE TABLE IF NOT EXISTS Logins (UserID TEXT, AltUserID TEXT, AuthMethod TEXT, Email TEXT, PasswordHash BLOB, Salt BLOB, UserName TEXT, LastLoginTimestamp TEXT, IsActivated INTEGER)"))
            conn.execute(text("CREATE TABLE IF NOT EXISTS Activations (AltUserID TEXT, ActivationID TEXT)"))
            conn.execute(text("CREATE TABLE IF NOT EXISTS Workspaces (UserID TEXT, ContentType TEXT, ContentID TEXT)"))
            conn.execute(text("CREATE TABLE IF NOT EXISTS Trees (ContentID TEXT, ImportID TEXT, Name TEXT, Description TEXT, ClassTalents TEXT, SpecTalents TEXT)"))
            conn.execute(text("CREATE TABLE IF NOT EXISTS PresetTrees (ContentID TEXT, Name TEXT, Description TEXT, ClassTalents TEXT, SpecTalents TEXT)"))
            conn.execute(text("CREATE TABLE IF NOT EXISTS Loadouts (ContentID TEXT, ImportID TEXT, TreeID TEXT, Name TEXT, Description TEXT)"))
            conn.execute(text("CREATE TABLE IF NOT EXISTS Builds (ContentID TEXT, ImportID TEXT, TreeID TEXT, LoadoutID TEXT, Name TEXT, LevelCap INTEGER, UseLevelCap INTEGER, AssignedSkills TEXT)"))
            conn.execute(text("CREATE TABLE IF NOT EXISTS Talents (ContentID TEXT, OrderID INTEGER, NodeID INTEGER, TalentType TEXT, Name TEXT, NameSwitch TEXT, Description TEXT, DescriptionSwitch TEXT, Row INTEGER, Column INTEGER, MaxPoints INTEGER, RequiredPoints INTEGER, PreFilled INTEGER, ParentIDs TEXT, ChildIDs TEXT, IconName TEXT, IconNameSwitch TEXT)"))
            conn.execute(text("CREATE TABLE IF NOT EXISTS PresetTalents (ContentID TEXT, OrderID INTEGER, NodeID INTEGER, TalentType TEXT, Name TEXT, NameSwitch TEXT, Description TEXT, DescriptionSwitch TEXT, Row INTEGER, Column INTEGER, MaxPoints INTEGER, RequiredPoints INTEGER, PreFilled INTEGER, ParentIDs TEXT, ChildIDs TEXT, IconName TEXT, IconNameSwitch TEXT)"))
            conn.execute(text("CREATE TABLE IF NOT EXISTS Likes (UserID TEXT, ContentID TEXT, Timestamp TEXT)"))
            conn.execute(text("CREATE TABLE IF NOT EXISTS Comments (CommentID TEXT, UserID TEXT, ContentID TEXT, Message TEXT, Timestamp TEXT, ReplyID TEXT, WasEdited INTEGER)"))
            conn.execute(text("CREATE TABLE IF NOT EXISTS Feedbacks (FeedbackID TEXT, UserID TEXT, Message TEXT)"))
            # fmt: on

            conn.commit()

    def commit(self) -> None:
        with self.engine.connect() as conn:
            conn.commit()

    def execute_query(
        self, query: Query, expects_results: bool = False
    ) -> Sequence[Row[Any]]:
        with self.engine.connect() as conn:
            res = conn.execute(text(query.get_sql()))
            conn.commit()
        if expects_results:
            return res.all()

    def create_login(self, login: Login) -> None:
        q = Query.into("Logins").insert(
            login.user_id,
            login.alt_user_id,
            login.auth_method,
            login.email,
            Parameter(":pwhash"),
            Parameter(":salt"),
            login.user_name,
            login.last_login_timestamp,
            login.is_activated,
        )
        q: TextClause = text(q.get_sql())
        with self.engine.connect() as conn:
            conn.execute(
                q, parameters=dict(pwhash=login.password_hash, salt=login.salt)
            )
            conn.commit()

    def delete_login(self, user_id: str) -> None:
        table: Table = Table("Logins")
        q = Query.from_(table).where(table.UserID == user_id).delete()
        with self.engine.connect() as conn:
            conn.execute(text(q.get_sql()))
            conn.commit()

    def get_login(
        self,
        auth_method: Literal["USERID", "ALTUSERID", "USERNAMEEMAIL", "SSO"],
        user_id: Union[None, str] = None,
        user_name_email: Union[None, str] = None,
    ) -> Union[Login, None]:
        table: Table = Table("Logins")
        login_cols: tuple[str] = (
            "UserID",
            "AltUserID",
            "AuthMethod",
            "Email",
            "PasswordHash",
            "Salt",
            "UserName",
            "LastLoginTimestamp",
            "IsActivated",
        )
        if auth_method == "USERID":
            q = Query.from_(table).select(*login_cols).where((table.UserID == user_id))
        elif auth_method == "ALTUSERID":
            q = (
                Query.from_(table)
                .select(*login_cols)
                .where((table.AltUserID == user_id))
            )
        elif auth_method == "USERNAMEEMAIL":
            q = (
                Query.from_(table)
                .select(*login_cols)
                .where(
                    (table.UserName == user_name_email)
                    | (table.Email == user_name_email)
                )
            )
        elif auth_method == "SSO":
            q = (
                Query.from_(table)
                .select(*login_cols)
                .where((table.Email == user_name_email))
            )
        with self.engine.connect() as conn:
            res = conn.execute(text(q.get_sql())).first()
        return None if res is None else Login(*res)

    def add_login_variant_to_user(self) -> None:
        # TODO: Add possibility to extend login information to both login methods
        raise Exception("Not yet implemented!")

    def activate_login(self, alt_user_id: str) -> None:
        table: Table = Table("Logins")
        q = (
            Query.update(table)
            .set(table.IsActivated, True)
            .where(table.AltUserID == alt_user_id)
        )
        with self.engine.connect() as conn:
            conn.execute(text(q.get_sql()))
            conn.commit()

    def create_activation(self, activation: Activation) -> None:
        q = Query.into("Activations").insert(
            activation.alt_user_id, activation.activation_id
        )
        with self.engine.connect() as conn:
            conn.execute(text(q.get_sql()))
            conn.commit()

    def delete_activation(self, alt_user_id: str) -> None:
        table: Table = Table("Activations")
        q = Query.from_(table).where(table.AltUserID == alt_user_id).delete()
        with self.engine.connect() as conn:
            conn.execute(text(q.get_sql()))
            conn.commit()

    def get_activation(self, activation_id: str) -> Union[Activation, None]:
        table: Table = Table("Activations")
        q = (
            Query.from_(table)
            .select("AltUserID", "ActivationID")
            .where((table.ActivationID == activation_id))
        )
        with self.engine.connect() as conn:
            res = conn.execute(text(q.get_sql())).first()
        return None if res is None else Activation(*res)

    def create_workspace(self, workspace: Workspace) -> None:
        with self.engine.connect() as conn:
            for item in workspace.items:
                q = Query.into("Workspaces").insert(*item)
                conn.execute(text(q.get_sql()))
            conn.commit()

    def delete_workspace(self, user_id: str) -> None:
        table: Table = Table("Workspaces")
        q = Query.from_(table).where(table.UserID == user_id).delete()
        with self.engine.connect() as conn:
            conn.execute(text(q.get_sql()))
            conn.commit()

    def get_workspace(self, user_id: str) -> Union[Workspace, None]:
        table: Table = Table("Workspaces")
        q = (
            Query.from_(table)
            .select("UserID", "ContentType", "ContentID")
            .where((table.UserID == user_id))
        )
        with self.engine.connect() as conn:
            res = conn.execute(text(q.get_sql())).all()
        return Workspace(items=res)

    # this is used in other views to check if a user has access to a content_id
    # in case a logged in user does manual api calls
    def validate_content_access(self, user_id: str, content_id: str) -> bool:
        table: Table = Table("Workspaces")
        q = (
            Query.from_(table)
            .select("UserID")
            .where((table.UserID == user_id) | (table.ContentID == content_id))
        )
        with self.engine.connect() as conn:
            res = conn.execute(text(q.get_sql())).first()
        return res is not None

    def create_tree(self, tree: Tree, custom: bool = True) -> None:
        table: Table = Table("Trees" if custom else "PresetTrees")
        if custom:
            q = Query.into(table).insert(
                tree.content_id,
                tree.import_id,
                tree.name,
                tree.description,
                tree.class_talents,
                tree.spec_talents,
            )
        else:
            q = Query.into(table).insert(
                tree.content_id,
                tree.name,
                tree.description,
                tree.class_talents,
                tree.spec_talents,
            )
        with self.engine.connect() as conn:
            conn.execute(text(q.get_sql()))
            conn.commit()

    def create_trees(self, trees: list[Tree], custom: bool = True) -> None:
        table: Table = Table("Trees" if custom else "PresetTrees")
        with self.engine.connect() as conn:
            for tree in trees:
                if custom:
                    q = Query.into(table).insert(
                        tree.content_id,
                        tree.import_id,
                        tree.name,
                        tree.description,
                        tree.class_talents,
                        tree.spec_talents,
                    )
                else:
                    q = Query.into(table).insert(
                        tree.content_id,
                        tree.name,
                        tree.description,
                        tree.class_talents,
                        tree.spec_talents,
                    )

                conn.execute(text(q.get_sql()))
            conn.commit()

    def delete_tree(self, content_id: str, custom: bool = True) -> None:
        table: Table = Table("Trees" if custom else "PresetTrees")
        q = Query.from_(table).where(table.ContentID == content_id).delete()
        with self.engine.connect() as conn:
            conn.execute(text(q.get_sql()))
            conn.commit()

    def get_tree(
        self, content_id: str, custom: Union[bool, None] = None
    ) -> Union[Tree, None]:
        res = None
        auto_custom = True
        if custom == True or custom is None:
            table: Table = Table("Trees")
            q = (
                Query.from_(table)
                .select(
                    "ContentID",
                    "ImportID",
                    "Name",
                    "Description",
                    "ClassTalents",
                    "SpecTalents",
                )
                .where((table.ContentID == content_id))
            )
            with self.engine.connect() as conn:
                res = conn.execute(text(q.get_sql())).first()
        if custom == False or (custom is None and res is None):
            table: Table = Table("PresetTrees")
            auto_custom = False
            q = (
                Query.from_(table)
                .select(
                    "ContentID", "Name", "Description", "ClassTalents", "SpecTalents"
                )
                .where((table.ContentID == content_id))
            )
            with self.engine.connect() as conn:
                res = conn.execute(text(q.get_sql())).first()
        if res is None:
            return None
        else:
            if custom or auto_custom:
                return Tree(*res)
            else:
                return Tree(res[0], None, res[1], res[2], res[3], res[4])

    def update_tree(self, tree: Tree, custom: bool = True) -> None:
        table: Table = Table("Trees" if custom else "PresetTrees")
        if custom:
            q = (
                Query.update(table)
                .set(table.ImportID, tree.import_id)
                .set(table.Name, tree.name)
                .set(table.Description, tree.description)
                .set(table.ClassTalents, tree.class_talents)
                .set(table.SpecTalents, tree.spec_talents)
                .where(table.ContentID == tree.content_id)
            )
        else:
            q = (
                Query.update(table)
                .set(table.Name, tree.name)
                .set(table.Description, tree.description)
                .set(table.ClassTalents, tree.class_talents)
                .set(table.SpecTalents, tree.spec_talents)
                .where(table.ContentID == tree.content_id)
            )
        with self.engine.connect() as conn:
            conn.execute(text(q.get_sql()))
            conn.commit()

    def create_loadout(self, loadout: Loadout) -> None:
        q = Query.into("Loadouts").insert(
            loadout.content_id,
            loadout.import_id,
            loadout.tree_id,
            loadout.name,
            loadout.description,
        )
        with self.engine.connect() as conn:
            conn.execute(text(q.get_sql()))
            conn.commit()

    def delete_loadout(self, content_id: str) -> None:
        table: Table = Table("Loadouts")
        q = Query.from_(table).where(table.ContentID == content_id).delete()
        with self.engine.connect() as conn:
            conn.execute(text(q.get_sql()))
            conn.commit()

    def get_loadout(self, content_id: str) -> Union[Loadout, None]:
        table: Table = Table("Loadouts")
        q = (
            Query.from_(table)
            .select("ContentID", "ImportID", "TreeID", "Name", "Description")
            .where((table.ContentID == content_id))
        )
        with self.engine.connect() as conn:
            res = conn.execute(text(q.get_sql())).first()
        return None if res is None else Loadout(*res)

    def update_loadout(self, loadout: Loadout) -> None:
        table: Table = Table("Loadouts")
        q = (
            Query.update(table)
            .set(table.Name, loadout.name)
            .set(table.Description, loadout.description)
            .where(table.ContentID == loadout.content_id)
        )
        with self.engine.connect() as conn:
            conn.execute(text(q.get_sql()))
            conn.commit()

    def create_build(self, build: Build) -> None:
        q = Query.into("Builds").insert(
            build.content_id,
            build.import_id,
            build.tree_id,
            build.loadout_id,
            build.name,
            build.level_cap,
            build.use_level_cap,
            build.assigned_skills,
        )
        with self.engine.connect() as conn:
            conn.execute(text(q.get_sql()))
            conn.commit()

    def delete_build(self, content_id: str) -> None:
        table: Table = Table("Builds")
        q = Query.from_(table).where(table.ContentID == content_id).delete()
        with self.engine.connect() as conn:
            conn.execute(text(q.get_sql()))
            conn.commit()

    def get_build(self, content_id: str) -> Union[Build, None]:
        table: Table = Table("Builds")
        q = (
            Query.from_(table)
            .select(
                "ContentID",
                "ImportID",
                "TreeID",
                "LoadoutID",
                "Name",
                "LevelCap",
                "UseLevelCap",
                "AssignedSkills",
            )
            .where((table.ContentID == content_id))
        )
        with self.engine.connect() as conn:
            res = conn.execute(text(q.get_sql())).first()
        return None if res is None else Build(*res)

    def update_build(self, build: Build) -> None:
        table: Table = Table("Builds")
        q = (
            Query.update(table)
            .set(table.Name, build.name)
            .set(table.LevelCap, build.level_cap)
            .set(table.UseLevelCap, build.use_level_cap)
            .set(table.AssignedSkills, build.assigned_skills)
            .where(table.ContentID == build.content_id)
        )
        with self.engine.connect() as conn:
            conn.execute(text(q.get_sql()))
            conn.commit()

    def create_talent(self, talent: Talent, custom: bool) -> None:
        table: Table = Table("Talents" if custom else "PresetTalents")
        q = Query.into(table).insert(*vars(talent).values())
        with self.engine.connect() as conn:
            conn.execute(text(q.get_sql()))
            conn.commit()

    def create_talents(self, talents: list[Talent], custom: bool) -> None:
        table: Table = Table("Talents" if custom else "PresetTalents")
        with self.engine.connect() as conn:
            for talent in talents:
                q = Query.into(table).insert(*vars(talent).values())
                conn.execute(text(q.get_sql()))
            conn.commit()

    def delete_talent(self, content_id: str, custom: bool) -> None:
        table: Table = Table("Talents" if custom else "PresetTalents")
        q = Query.from_(table).where(table.ContentID == content_id).delete()
        with self.engine.connect() as conn:
            conn.execute(text(q.get_sql()))
            conn.commit()

    def get_talent(self, content_id: str, custom: bool) -> Union[Talent, None]:
        table: Table = Table("Talents" if custom else "PresetTalents")
        q = (
            Query.from_(table)
            .select(
                "ContentID",
                "OrderID",
                "NodeID",
                "TalentType",
                "Name",
                "NameSwitch",
                "Description",
                "DescriptionSwitch",
                "Row",
                "Column",
                "MaxPoints",
                "RequiredPoints",
                "PreFilled",
                "ParentIDs",
                "ChildIDs",
                "IconName",
                "IconNameSwitch",
            )
            .where((table.ContentID == content_id))
        )
        with self.engine.connect() as conn:
            res = conn.execute(text(q.get_sql())).first()
        return None if res is None else Talent(*res)

    def get_talents(
        self, content_ids: list[str], custom: Union[bool, None]
    ) -> list[Talent]:
        talent_list: list = []
        talent_columns = (
            "ContentID",
            "OrderID",
            "NodeID",
            "TalentType",
            "Name",
            "NameSwitch",
            "Description",
            "DescriptionSwitch",
            "Row",
            "Column",
            "MaxPoints",
            "RequiredPoints",
            "PreFilled",
            "ParentIDs",
            "ChildIDs",
            "IconName",
            "IconNameSwitch",
        )
        with self.engine.connect() as conn:
            if custom == True or custom is None:
                table: Table = Table("Talents")
                q = (
                    Query.from_(table)
                    .select(*talent_columns)
                    .where(table.ContentID.isin(content_ids))
                )
                res = conn.execute(text(q.get_sql())).all()
                talent_list += res
            if custom == False or len(talent_list) < len(content_ids):
                table: Table = Table("PresetTalents")
                q = (
                    Query.from_(table)
                    .select(*talent_columns)
                    .where(table.ContentID.isin(content_ids))
                )
                res = conn.execute(text(q.get_sql())).all()
                talent_list += res
        return [Talent(*x) for x in talent_list]

    def update_talent(self, talent: Talent, custom: bool) -> None:
        table: Table = Table("Talents" if custom else "PresetTalents")
        q = (
            Query.update(table)
            .set(table.OrderID, talent.order_id)
            .set(table.NodeID, talent.node_id)
            .set(table.TalentType, talent.talent_type)
            .set(table.Name, talent.name)
            .set(table.NameSwitch, talent.name_switch)
            .set(table.Description, talent.description)
            .set(table.DescriptionSwitch, talent.description_switch)
            .set(table.Row, talent.row)
            .set(table.Column, talent.column)
            .set(table.MaxPoints, talent.max_points)
            .set(table.RequiredPoints, talent.required_points)
            .set(table.PreFilled, talent.pre_filled)
            .set(table.ParentIDs, talent.parent_ids)
            .set(table.ChildIDs, talent.child_ids)
            .set(table.IconName, talent.icon_name)
            .set(table.IconNameSwitch, talent.icon_name_switch)
            .where(table.ContentID == talent.content_id)
        )
        with self.engine.connect() as conn:
            conn.execute(text(q.get_sql()))
            conn.commit()

    def create_like(self, like: Like) -> None:
        q = Query.into("Likes").insert(like.user_id, like.content_id, like.timestamp)
        with self.engine.connect() as conn:
            conn.execute(text(q.get_sql()))
            conn.commit()

    def delete_like(self, user_id: str, content_id: str) -> None:
        table: Table = Table("Likes")
        q = (
            Query.from_(table)
            .where((table.ContentID == content_id) & (table.UserID == user_id))
            .delete()
        )
        with self.engine.connect() as conn:
            conn.execute(text(q.get_sql()))
            conn.commit()

    def get_likes(
        self, user_id: Union[str, None], content_id: Union[str, None]
    ) -> list[Like]:
        if user_id is None and content_id is None:
            raise ValueError("user_id and content_id cannot both be None!")
        table: Table = Table("Likes")
        if user_id is not None and content_id is not None:
            q = (
                Query.from_(table)
                .select("UserID", "ContentID", "Timestamp")
                .where((table.UserID == user_id) & (table.ContentID == content_id))
            )
        elif user_id is not None and content_id is None:
            q = (
                Query.from_(table)
                .select("UserID", "ContentID", "Timestamp")
                .where((table.UserID == user_id))
            )
        else:
            q = (
                Query.from_(table)
                .select("UserID", "ContentID", "Timestamp")
                .where((table.ContentID == content_id))
            )
        with self.engine.connect() as conn:
            res = conn.execute(text(q.get_sql()))
            likes: list(Like) = []
            for row in res.all():
                likes.append(Like(*row))
        return likes

    def create_comment(self, comment: Comment) -> None:
        q = Query.into("Comments").insert(
            comment.comment_id,
            comment.user_id,
            comment.content_id,
            comment.message,
            comment.timestamp,
            comment.reply_id,
            comment.was_edited,
        )
        with self.engine.connect() as conn:
            conn.execute(text(q.get_sql()))
            conn.commit()

    def delete_comment(self, comment_id: str) -> None:
        table: Table = Table("Comments")
        q = Query.from_(table).where((table.CommentID == comment_id)).delete()
        with self.engine.connect() as conn:
            conn.execute(text(q.get_sql()))
            conn.commit()

    def get_comment(self, comment_id: str) -> Union[Comment, None]:
        table: Table = Table("Comments")
        q = (
            Query.from_(table)
            .select(
                "CommentID",
                "UserID",
                "ContentID",
                "Message",
                "Timestamp",
                "ReplyID",
                "WasEdited",
            )
            .where((table.CommentID == comment_id))
        )
        with self.engine.connect() as conn:
            res = conn.execute(text(q.get_sql())).first()
        return None if res is None else Comment(*res)

    def get_comments(
        self, user_id: Union[str, None], content_id: Union[str, None]
    ) -> list[Comment]:
        if user_id is None and content_id is None:
            raise ValueError("user_id and content_id cannot both be None!")
        table: Table = Table("Comments")
        if user_id is not None and content_id is not None:
            q = (
                Query.from_(table)
                .select(
                    "CommentID",
                    "UserID",
                    "ContentID",
                    "Message",
                    "Timestamp",
                    "ReplyID",
                    "WasEdited",
                )
                .where((table.UserID == user_id) & (table.ContentID == content_id))
            )
        elif user_id is not None and content_id is None:
            q = (
                Query.from_(table)
                .select(
                    "CommentID",
                    "UserID",
                    "ContentID",
                    "Message",
                    "Timestamp",
                    "ReplyID",
                    "WasEdited",
                )
                .where((table.UserID == user_id))
            )
        else:
            q = (
                Query.from_(table)
                .select(
                    "CommentID",
                    "UserID",
                    "ContentID",
                    "Message",
                    "Timestamp",
                    "ReplyID",
                    "WasEdited",
                )
                .where((table.ContentID == content_id))
            )
        with self.engine.connect() as conn:
            res = conn.execute(text(q.get_sql()))
            comments: list(Comment) = []
            for row in res.all():
                comments.append(Like(*row))
        return comments

    def update_comment(self, comment: Comment) -> None:
        table: Table = Table("Comments")
        q = (
            Query.update(table)
            .set(table.Message, comment.message)
            .set(table.Timestamp, comment.timestamp)
            .set(table.WasEdited, True)
            .where(table.comment_id == comment.comment_id)
        )
        with self.engine.connect() as conn:
            conn.execute(text(q.get_sql()))
            conn.commit()

    def create_feedback(self, feedback: Feedback) -> None:
        q = Query.into("Feedbacks").insert(
            feedback.feedback_id, feedback.user_id, feedback.message
        )
        with self.engine.connect() as conn:
            conn.execute(text(q.get_sql()))
            conn.commit()


if __name__ == "__main__":
    from sqlalchemy import create_engine

    db_path = os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        "database",
        "data.sqlite",
    )
    engine = create_engine(f"sqlite+pysqlite:///{db_path}", echo=True)
    db_handler: DBHandler = DBHandler(engine)
    db_handler.initialize_databases(reset=True)
