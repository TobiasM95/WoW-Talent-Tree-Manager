import sys
import os
import sqlite3


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
        self.cursor.execute("CREATE TABLE IF NOT EXISTS Logins (UserID TEXT, AuthMethod TEXT, Email TEXT, PasswordHash TEXT, Salt TEXT, UserName TEXT)")
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


if __name__ == "__main__":
    db_handler: DBHandler = DBHandler()
    db_handler.initialize_databases()
