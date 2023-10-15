import sys
import os
import sqlite3
from typing import Literal, Union
import datetime
from pypika import Table, Query, Database
from sqlalchemy import Engine, text


class Login:
    def __init__(
        self,
        user_id: str,
        alt_user_id: str,
        auth_method: Literal["PW", "SSO"],
        email: str,
        password_hash: str,
        salt: str,
        user_name: str,
        last_login_timestamp: datetime.datetime,
        is_activated: bool,
    ):
        self.user_id: str = user_id
        self.alt_user_id: str = alt_user_id
        self.auth_method: Literal["PW", "SSO"] = auth_method
        self.email: str = email
        self.password_hash: str = password_hash
        self.salt: str = salt
        self.user_name: str = user_name
        self.last_login_timestamp: datetime.datetime = last_login_timestamp
        self.is_activated: bool = is_activated


class Workspace:
    def __init__(
        self,
        user_id: str,
        content_type: Literal["tree", "loadout", "build"],
        content_id: str,
    ):
        self.user_id: str = user_id
        self.content_type: Literal["tree", "loadout", "build"] = content_type
        self.content_id: str = content_id


class Tree:
    def __init__(self, content_id: str, tree_string: str):
        self.content_id: str = content_id
        self.tree_string: str = tree_string


class Loadout:
    def __init__(self, content_id: str, tree_id: str, loadout_string: str):
        self.content_id: str = content_id
        self.tree_id: str = tree_id
        self.loadout_string: str = loadout_string


class Build:
    def __init__(self, content_id: str, tree_id: str, loadout_id: str, build_string):
        self.content_id: str = content_id
        self.tree_id: str = tree_id
        self.loadout_id: str = loadout_id
        self.build_string: str = build_string


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
            conn.execute(text("CREATE TABLE IF NOT EXISTS Logins (UserID TEXT, AltUserID TEXT, AuthMethod TEXT, Email TEXT, PasswordHash TEXT, Salt BLOB, UserName TEXT, LastLoginTimestamp TEXT, IsActivated INTEGER)"))
            conn.execute(text("CREATE TABLE IF NOT EXISTS Workspaces (UserID TEXT, ContentType TEXT, ContentID TEXT)"))
            conn.execute(text("CREATE TABLE IF NOT EXISTS Trees (ContentID TEXT, TreeString TEXT)"))
            conn.execute(text("CREATE TABLE IF NOT EXISTS Loadouts (ContentID TEXT, TreeID TEXT, LoadoutString TEXT)"))
            conn.execute(text("CREATE TABLE IF NOT EXISTS Builds (ContentID TEXT, TreeID TEXT, LoadoutID TEXT, BuildString TEXT)"))
            conn.execute(text("CREATE TABLE IF NOT EXISTS Likes (UserID TEXT, ContentID TEXT, Timestamp TEXT)"))
            conn.execute(text("CREATE TABLE IF NOT EXISTS Comments (CommentID TEXT, UserID TEXT, ContentID TEXT, Message TEXT, Timestamp TEXT, ReplyID TEXT, WasEdited INTEGER)"))
            conn.execute(text("CREATE TABLE IF NOT EXISTS Feedbacks (FeedbackID TEXT, UserID TEXT, Message TEXT)"))
            # fmt: on

            conn.commit()

        self._load_in_presets()

    def _load_in_presets(self) -> None:
        # TODO: Load in tree presets
        pass

    def commit(self) -> None:
        with self.engine.connect() as conn:
            conn.commit()

    def create_login(self, login: Login) -> None:
        q = Query.into("Logins").insert(
            login.user_id,
            login.alt_user_id,
            login.auth_method,
            login.email,
            login.password_hash,
            login.salt,
            login.user_name,
            login.last_login_timestamp,
            login.is_activated,
        )
        with self.engine.connect() as conn:
            conn.execute(q.get_sql())
            conn.commit()

    def delete_login(self, user_id: str) -> None:
        table: Table = Table("Logins")
        q = Query.from_(table).where(table.UserID == user_id).delete()
        with self.engine.connect() as conn:
            conn.execute(q.get_sql())
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

    def add_login_variant_to_user(self):
        # TODO: Add possibility to extend login information to both login methods
        raise Exception("Not yet implemented!")

    def create_workspace(self, workspace: Workspace) -> None:
        q = Query.into("Workspaces").insert(
            workspace.user_id, workspace.content_type, workspace.content_id
        )
        with self.engine.connect() as conn:
            conn.execute(q.get_sql())
            conn.commit()

    def delete_workspace(self, user_id: str) -> None:
        table: Table = Table("Workspaces")
        q = Query.from_(table).where(table.UserID == user_id).delete()
        with self.engine.connect() as conn:
            conn.execute(q.get_sql())
            conn.commit()

    def get_workspace(self, user_id: str) -> Union[Workspace, None]:
        table: Table = Table("Workspaces")
        q = (
            Query.from_(table)
            .select("UserID", "ContentType", "ContentID")
            .where((table.UserID == user_id))
        )
        with self.engine.connect() as conn:
            res = conn.execute(q.get_sql()).first()
        return None if res is None else Workspace(*res)

    def create_tree(self, tree: Tree) -> None:
        q = Query.into("Trees").insert(tree.content_id, tree.tree_string)
        with self.engine.connect() as conn:
            conn.execute(q.get_sql())
            conn.commit()

    def delete_tree(self, content_id: str) -> None:
        table: Table = Table("Trees")
        q = Query.from_(table).where(table.ContentID == content_id).delete()
        with self.engine.connect() as conn:
            conn.execute(q.get_sql())
            conn.commit()

    def get_tree(self, content_id: str) -> Union[Tree, None]:
        table: Table = Table("Trees")
        q = (
            Query.from_(table)
            .select("ContentID", "TreeString")
            .where((table.ContentID == content_id))
        )
        with self.engine.connect() as conn:
            res = conn.execute(q.get_sql()).first()
        return None if res is None else Tree(*res)

    def update_tree(self, tree: Tree) -> None:
        table: Table = Table("Trees")
        q = (
            Query.update(table)
            .set(table.TreeString, tree.tree_string)
            .where(table.ContentID == tree.content_id)
        )
        with self.engine.connect() as conn:
            conn.execute(q.get_sql())
            conn.commit()

    def create_loadout(self, loadout: Loadout) -> None:
        q = Query.into("Loadouts").insert(
            loadout.content_id, loadout.tree_id, loadout.loadout_string
        )
        with self.engine.connect() as conn:
            conn.execute(q.get_sql())
            conn.commit()

    def delete_loadout(self, content_id: str) -> None:
        table: Table = Table("Loadouts")
        q = Query.from_(table).where(table.ContentID == content_id).delete()
        with self.engine.connect() as conn:
            conn.execute(q.get_sql())
            conn.commit()

    def get_loadout(self, content_id: str) -> Union[Loadout, None]:
        table: Table = Table("Loadouts")
        q = (
            Query.from_(table)
            .select("ContentID", "TreeID", "LoadoutString")
            .where((table.ContentID == content_id))
        )
        with self.engine.connect() as conn:
            res = conn.execute(q.get_sql()).first()
        return None if res is None else Loadout(*res)

    def update_loadout(self, loadout: Loadout) -> None:
        table: Table = Table("Loadouts")
        q = (
            Query.update(table)
            .set(table.LoadoutString, loadout.loadout_string)
            .where(table.ContentID == loadout.content_id)
        )
        with self.engine.connect() as conn:
            conn.execute(q.get_sql())
            conn.commit()

    def create_build(self, build: Build) -> None:
        q = Query.into("Builds").insert(
            build.content_id, build.tree_id, build.loadout_id, build.build_string
        )
        with self.engine.connect() as conn:
            conn.execute(q.get_sql())
            conn.commit()

    def delete_build(self, content_id: str) -> None:
        table: Table = Table("Builds")
        q = Query.from_(table).where(table.ContentID == content_id).delete()
        with self.engine.connect() as conn:
            conn.execute(q.get_sql())
            conn.commit()

    def get_build(self, content_id: str) -> Union[Build, None]:
        table: Table = Table("Builds")
        q = (
            Query.from_(table)
            .select("ContentID", "TreeID", "LoadoutID", "BuildString")
            .where((table.ContentID == content_id))
        )
        with self.engine.connect() as conn:
            res = conn.execute(q.get_sql()).first()
        return None if res is None else Login(*res)

    def update_Build(self, build: Build) -> None:
        table: Table = Table("Builds")
        q = (
            Query.update(table)
            .set(table.LoadoutString, build.build_string)
            .where(table.ContentID == build.content_id)
        )
        with self.engine.connect() as conn:
            conn.execute(q.get_sql())
            conn.commit()

    def create_like(self, like: Like) -> None:
        q = Query.into("Likes").insert(like.user_id, like.content_id, like.timestamp)
        with self.engine.connect() as conn:
            conn.execute(q.get_sql())
            conn.commit()

    def delete_like(self, user_id: str, content_id: str) -> None:
        table: Table = Table("Likes")
        q = (
            Query.from_(table)
            .where((table.ContentID == content_id) & (table.UserID == user_id))
            .delete()
        )
        with self.engine.connect() as conn:
            conn.execute(q.get_sql())
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
            res = conn.execute(q.get_sql())
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
            conn.execute(q.get_sql())
            conn.commit()

    def delete_comment(self, comment_id: str) -> None:
        table: Table = Table("Comments")
        q = Query.from_(table).where((table.CommentID == comment_id)).delete()
        with self.engine.connect() as conn:
            conn.execute(q.get_sql())
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
            res = conn.execute(q.get_sql()).first()
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
            res = conn.execute(q.get_sql())
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
            conn.execute(q.get_sql())
            conn.commit()

    def create_feedback(self, feedback: Feedback) -> None:
        q = Query.into("Feedbacks").insert(
            feedback.feedback_id, feedback.user_id, feedback.message
        )
        with self.engine.connect() as conn:
            conn.execute(q.get_sql())
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
