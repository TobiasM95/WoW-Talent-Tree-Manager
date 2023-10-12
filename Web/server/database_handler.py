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
    ):
        self.comment_id: str = comment_id
        self.user_id: str = user_id
        self.content_id: str = content_id
        self.message: str = message
        self.timestamp: datetime.datetime = timestamp
        self.reply_id: Union[None, str] = reply_id


class Feedback:
    def __init__(self, feedback_id: str, user_id: str, message_id: str):
        self.feedback_id: str = feedback_id
        self.user_id: str = user_id
        self.message_id: str = message_id


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

    def initialize_databases(self) -> None:
        is_initialized: bool = (
            self.cursor.execute(
                "SELECT count(*) FROM sqlite_master WHERE type='table'"
            ).fetchone()[0]
            == 8
        )

        if is_initialized:
            print("Database already initialized! Skip..")
            return

        # fmt: off
        self.cursor.execute("CREATE TABLE IF NOT EXISTS Logins (UserID TEXT, AuthMethod TEXT, Email TEXT, PasswordHash TEXT, Salt TEXT, UserName TEXT, LastLoginTimestamp TEXT)")
        self.cursor.execute("CREATE TABLE IF NOT EXISTS Workspaces (UserID TEXT, ContentType TEXT, ContentID TEXT)")
        self.cursor.execute("CREATE TABLE IF NOT EXISTS Trees (ContentID TEXT, TreeString TEXT)")
        self.cursor.execute("CREATE TABLE IF NOT EXISTS Loadouts (ContentID TEXT, TreeID TEXT, LoadoutString TEXT)")
        self.cursor.execute("CREATE TABLE IF NOT EXISTS Builds (ContentID TEXT, TreeID TEXT, LoadoutID TEXT, BuildString TEXT)")
        self.cursor.execute("CREATE TABLE IF NOT EXISTS Likes (UserID TEXT, ContentID TEXT, Timestamp TEXT)")
        self.cursor.execute("CREATE TABLE IF NOT EXISTS Comments (CommentID TEXT, UserID TEXT, ContentID TEXT, Message TEXT, Timestamp TEXT, ReplyID TEXT)")
        self.cursor.execute("CREATE TABLE IF NOT EXISTS Feedbacks (FeedbackID TEXT, UserID TEXT, Message TEXT)")
        # fmt: on

        self.load_in_presets()

        self.connection.commit()

    def _load_in_presets(self) -> None:
        # tree presets
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
    ):
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


if __name__ == "__main__":
    db_handler: DBHandler = DBHandler()
    db_handler.initialize_databases()
