import sys
import os
import sqlite3
from typing import Literal, Union
import datetime
from pypika import Table, Query, Database


class Login:
    def __init__(
        self,
        user_id: str,
        auth_method: Literal["PW", "SSIO"],
        email: str,
        password_hash: str,
        salt: str,
        user_name: str,
        last_login_timestamp: datetime.datetime,
    ):
        self.user_id: str = user_id
        self.auth_method: Literal["PW", "SSIO"] = auth_method
        self.email: str = email
        self.password_hash: str = password_hash
        self.salt: str = salt
        self.user_name: str = user_name
        self.last_login_timestamp: datetime.datetime = last_login_timestamp


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
    def __init__(self) -> None:
        self.connection: sqlite3.Connection = sqlite3.connect(
            os.path.join(
                os.path.dirname(os.path.abspath(__file__)),
                "database",
                "data.sqlite",
            )
        )
        self.cursor: sqlite3.Cursor = self.connection.cursor()

    def initialize_databases(self, reset: bool = False) -> None:
        is_initialized: bool = (
            self.cursor.execute(
                "SELECT count(*) FROM sqlite_master WHERE type='table'"
            ).fetchone()[0]
            == 8
        )

        if is_initialized and not reset:
            print("Database already initialized! Skip..")
            return

        if reset:
            self.cursor.close()
            self.connection.close()
            db_path = os.path.join(
                os.path.dirname(os.path.abspath(__file__)),
                "database",
                "data.sqlite",
            )
            if os.path.isfile(db_path):
                os.remove(db_path)
            self.__init__()

        # fmt: off
        self.cursor.execute("CREATE TABLE IF NOT EXISTS Logins (UserID TEXT, AuthMethod TEXT, Email TEXT, PasswordHash TEXT, Salt TEXT, UserName TEXT, LastLoginTimestamp TEXT)")
        self.cursor.execute("CREATE TABLE IF NOT EXISTS Workspaces (UserID TEXT, ContentType TEXT, ContentID TEXT)")
        self.cursor.execute("CREATE TABLE IF NOT EXISTS Trees (ContentID TEXT, TreeString TEXT)")
        self.cursor.execute("CREATE TABLE IF NOT EXISTS Loadouts (ContentID TEXT, TreeID TEXT, LoadoutString TEXT)")
        self.cursor.execute("CREATE TABLE IF NOT EXISTS Builds (ContentID TEXT, TreeID TEXT, LoadoutID TEXT, BuildString TEXT)")
        self.cursor.execute("CREATE TABLE IF NOT EXISTS Likes (UserID TEXT, ContentID TEXT, Timestamp TEXT)")
        self.cursor.execute("CREATE TABLE IF NOT EXISTS Comments (CommentID TEXT, UserID TEXT, ContentID TEXT, Message TEXT, Timestamp TEXT, ReplyID TEXT, WasEdited INTEGER)")
        self.cursor.execute("CREATE TABLE IF NOT EXISTS Feedbacks (FeedbackID TEXT, UserID TEXT, Message TEXT)")
        # fmt: on

        self.load_in_presets()

        self.connection.commit()

    def _load_in_presets(self) -> None:
        # TODO: Load in tree presets
        pass

    def commit(self) -> None:
        self.connection.commit()

    def create_login(self, login: Login, commit: bool = True) -> None:
        q = Query.into("Logins").insert(
            login.user_id,
            login.auth_method,
            login.email,
            login.password_hash,
            login.salt,
            login.user_name,
            login.last_login_timestamp,
        )
        self.cursor.execute(q.get_sql())
        if commit:
            self.connection.commit()

    def delete_login(self, user_id: str, commit: bool = True) -> None:
        table: Table = Table("Logins")
        q = Query.from_(table).where(table.UserID == user_id).delete()
        self.cursor.execute(q.get_sql())
        if commit:
            self.connection.commit()

    def get_user_info_from_login(
        self,
        auth_method: Literal["USERPW", "EMAILPW", "SSIO"],
        user_name: Union[None, str],
        email: Union[None, str],
        password_hash: Union[None, str],
    ) -> tuple[str, str]:
        table: Table = Table("Logins")
        if auth_method == "USERPW":
            q = (
                Query.from_(table)
                .select("UserID", "UserName")
                .where(
                    (table.UserName == user_name)
                    & (table.PasswordHash == password_hash)
                )
            )
        elif auth_method == "EMAILPW":
            q = (
                Query.from_(table)
                .select("UserID", "UserName")
                .where((table.Email == email) & (table.PasswordHash == password_hash))
            )
        elif auth_method == "SSIO":
            q = (
                Query.from_(table)
                .select("UserID", "UserName")
                .where((table.Email == email))
            )
        user_id, user_name = self.cursor.execute(q.get_sql()).fetchone()

        return user_id, user_name

    def add_login_variant_to_user(self):
        # TODO: Add possibility to extend login information to both login methods
        raise Exception("Not yet implemented!")

    def create_workspace(self, workspace: Workspace, commit: bool = True) -> None:
        q = Query.into("Workspaces").insert(
            workspace.user_id, workspace.content_type, workspace.content_id
        )
        self.cursor.execute(q.get_sql())
        if commit:
            self.connection.commit()

    def delete_workspace(self, user_id: str, commit: bool) -> None:
        table: Table = Table("Workspaces")
        q = Query.from_(table).where(table.UserID == user_id).delete()
        self.cursor.execute(q.get_sql())
        if commit:
            self.connection.commit()

    def get_workspace(self, user_id: str) -> Workspace:
        table: Table = Table("Workspaces")
        q = (
            Query.from_(table)
            .select("UserID", "ContentType", "ContentID")
            .where((table.UserID == user_id))
        )
        return Workspace(*self.cursor.execute(q.get_sql()).fetchone())

    def create_tree(self, tree: Tree, commit: bool = True) -> None:
        q = Query.into("Trees").insert(tree.content_id, tree.tree_string)
        self.cursor.execute(q.get_sql())
        if commit:
            self.connection.commit()

    def delete_tree(self, content_id: str, commit: bool) -> None:
        table: Table = Table("Trees")
        q = Query.from_(table).where(table.ContentID == content_id).delete()
        self.cursor.execute(q.get_sql())
        if commit:
            self.connection.commit()

    def get_tree(self, content_id: str) -> Workspace:
        table: Table = Table("Trees")
        q = (
            Query.from_(table)
            .select("ContentID", "TreeString")
            .where((table.ContentID == content_id))
        )
        return Workspace(*self.cursor.execute(q.get_sql()).fetchone())

    def update_tree(self, tree: Tree, commit: bool = True) -> None:
        table: Table = Table("Trees")
        q = (
            Query.update(table)
            .set(table.TreeString, tree.tree_string)
            .where(table.ContentID == tree.content_id)
        )
        self.cursor.execute(q.get_sql())
        if commit:
            self.connection.commit()

    def create_loadout(self, loadout: Loadout, commit: bool = True) -> None:
        q = Query.into("Loadouts").insert(
            loadout.content_id, loadout.tree_id, loadout.loadout_string
        )
        self.cursor.execute(q.get_sql())
        if commit:
            self.connection.commit()

    def delete_loadout(self, content_id: str, commit: bool) -> None:
        table: Table = Table("Loadouts")
        q = Query.from_(table).where(table.ContentID == content_id).delete()
        self.cursor.execute(q.get_sql())
        if commit:
            self.connection.commit()

    def get_loadout(self, content_id: str) -> Loadout:
        table: Table = Table("Loadouts")
        q = (
            Query.from_(table)
            .select("ContentID", "TreeID", "LoadoutString")
            .where((table.ContentID == content_id))
        )
        return Loadout(*self.cursor.execute(q.get_sql()).fetchone())

    def update_loadout(self, loadout: Loadout, commit: bool = True) -> None:
        table: Table = Table("Loadouts")
        q = (
            Query.update(table)
            .set(table.LoadoutString, loadout.loadout_string)
            .where(table.ContentID == loadout.content_id)
        )
        self.cursor.execute(q.get_sql())
        if commit:
            self.connection.commit()

    def create_build(self, build: Build, commit: bool = True) -> None:
        q = Query.into("Builds").insert(
            build.content_id, build.tree_id, build.loadout_id, build.build_string
        )
        self.cursor.execute(q.get_sql())
        if commit:
            self.connection.commit()

    def delete_build(self, content_id: str, commit: bool) -> None:
        table: Table = Table("Builds")
        q = Query.from_(table).where(table.ContentID == content_id).delete()
        self.cursor.execute(q.get_sql())
        if commit:
            self.connection.commit()

    def get_build(self, content_id: str) -> Build:
        table: Table = Table("Builds")
        q = (
            Query.from_(table)
            .select("ContentID", "TreeID", "LoadoutID", "BuildString")
            .where((table.ContentID == content_id))
        )
        return Build(*self.cursor.execute(q.get_sql()).fetchone())

    def update_Build(self, build: Build, commit: bool = True) -> None:
        table: Table = Table("Builds")
        q = (
            Query.update(table)
            .set(table.LoadoutString, build.build_string)
            .where(table.ContentID == build.content_id)
        )
        self.cursor.execute(q.get_sql())
        if commit:
            self.connection.commit()

    def create_like(self, like: Like, commit: bool = True) -> None:
        q = Query.into("Likes").insert(like.user_id, like.content_id, like.timestamp)
        self.cursor.execute(q.get_sql())
        if commit:
            self.connection.commit()

    def delete_like(self, user_id: str, content_id: str, commit: bool) -> None:
        table: Table = Table("Likes")
        q = (
            Query.from_(table)
            .where((table.ContentID == content_id) & (table.UserID == user_id))
            .delete()
        )
        self.cursor.execute(q.get_sql())
        if commit:
            self.connection.commit()

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
        self.cursor.execute(q.get_sql())
        likes: list(Like) = []
        for row in self.cursor:
            likes.append(Like(*row))
        return likes

    def create_comment(self, comment: Comment, commit: bool = True) -> None:
        q = Query.into("Comments").insert(
            comment.comment_id,
            comment.user_id,
            comment.content_id,
            comment.message,
            comment.timestamp,
            comment.reply_id,
            comment.was_edited,
        )
        self.cursor.execute(q.get_sql())
        if commit:
            self.connection.commit()

    def delete_comment(self, comment_id: str, commit: bool) -> None:
        table: Table = Table("Comments")
        q = Query.from_(table).where((table.CommentID == comment_id)).delete()
        self.cursor.execute(q.get_sql())
        if commit:
            self.connection.commit()

    def get_comment(self, comment_id: str) -> Comment:
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
        return Comment(*self.cursor.execute(q.get_sql()).fetchone())

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
        self.cursor.execute(q.get_sql())
        comments: list(Comment) = []
        for row in self.cursor:
            comments.append(Like(*row))
        return comments

    def update_Build(self, build: Build, commit: bool = True) -> None:
        table: Table = Table("Builds")
        q = (
            Query.update(table)
            .set(table.LoadoutString, build.build_string)
            .where(table.ContentID == build.content_id)
        )
        self.cursor.execute(q.get_sql())
        if commit:
            self.connection.commit()

    def create_feedback(self, feedback: Feedback, commit: bool = True) -> None:
        q = Query.into("Feedbacks").insert(
            feedback.feedback_id, feedback.user_id, feedback.message
        )
        self.cursor.execute(q.get_sql())
        if commit:
            self.connection.commit()


if __name__ == "__main__":
    db_handler: DBHandler = DBHandler()
    db_handler.initialize_databases()
