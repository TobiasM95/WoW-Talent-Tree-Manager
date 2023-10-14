import os
import hashlib
from typing import Union

from dotenv import load_dotenv
from flask import Flask, abort, request
from flask_login import LoginManager, UserMixin, login_user, login_required, logout_user
from sqlalchemy import create_engine
from database_handler import Login, DBHandler

load_dotenv()

db_path = os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    "database",
    "data.sqlite",
)
engine = create_engine(f"sqlite+pysqlite:///{db_path}", echo=True)
db_handler: DBHandler = DBHandler(engine)

app: Flask = Flask(__name__)
app.secret_key: str = os.environ["FLASK_SECRET_KEY"]

login_manager: LoginManager = LoginManager()
login_manager.session_protection = "strong"
login_manager.login_view = "login"
login_manager.login_message_category = "info"
login_manager.init_app(app)


class User(UserMixin):
    @classmethod
    def get_user(alt_user_id):
        login: Login = db_handler.get_login(
            auth_method="ALTUSERID", user_id=alt_user_id
        )
        if login is None:
            return None
        user: User = User()
        user.login: Login = login
        return user

    def __init__(self, login: Union[Login, None]):
        self.login: Union[Login, None] = login

    def get_id(self):
        if "login" not in vars(self) or self.login is None:
            return None
        return self.login.alt_user_id


def validate_login(data: dict) -> tuple[bool, Union[User, None]]:
    auth_method: str = data.get("auth_method")
    email: str = data.get("email")
    user_name: Union[str, None] = data.get("user_name")
    password: Union[str, None] = data.get("password")
    if auth_method == "SSIO":
        login_info: Union[Login, None] = db_handler.get_login(
            auth_method=auth_method, email=email
        )
        if login_info is None:
            return (False, None)
    else:
        if auth_method == "USERNAME":
            login_info: Union[Login, None] = db_handler.get_login(
                auth_method=auth_method,
                user_name=user_name,
            )
        elif auth_method == "EMAIL":
            login_info: Union[Login, None] = db_handler.get_login(
                auth_method=auth_method, email=email
            )
        if login_info is None:
            return (False, None)
        password_hash: str = hash_password(password, login_info.salt)
        if password_hash != login_info.password_hash:
            return (False, None)
    return (True, User(login_info))


def hash_password(password, salt):
    return hashlib.scrypt(password.encode(), salt=salt, n=2**14, r=8, p=1)


@login_manager.user_loader
def load_user(user_id):
    return User.get_user(user_id)


@app.route("/login", methods=["POST"])
def login():
    data = request.json
    login_validated, user = validate_login(data)
    if not login_validated:
        abort(403)

    login_user(user)

    return {"success": True}, 200


@app.route("/logout")
@login_required
def logout():
    logout_user()
    return {"success": True}, 200


@app.route("/")
def hello_world():
    return "<p>Hello, World!</p>"
