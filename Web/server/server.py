import os
import hashlib
import datetime
import uuid
from typing import Union

from dotenv import load_dotenv
from flask import Flask, abort, request, jsonify
from flask_jwt_extended import (
    create_access_token,
    get_jwt_identity,
    jwt_required,
    JWTManager,
    current_user,
    set_access_cookies,
    unset_jwt_cookies,
    get_jwt,
)
from sqlalchemy import create_engine
from database_handler import Login, DBHandler, Activation

load_dotenv()

db_path = os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    "database",
    "data.sqlite",
)
engine = create_engine(f"sqlite+pysqlite:///{db_path}", echo=True)
db_handler: DBHandler = DBHandler(engine)

app: Flask = Flask(__name__)
app.config["JWT_SECRET_KEY"]: str = os.environ["JWT_SECRET_KEY"]
app.config["JWT_TOKEN_LOCATION"] = ["cookies"]
app.config["JWT_COOKIE_SECURE"] = os.environ["JWT_COOKIE_SECURE"] == "True"
app.config["JWT_ACCESS_TOKEN_EXPIRES"] = datetime.timedelta(hours=1)

jwt = JWTManager(app)


def validate_login(data: dict) -> tuple[bool, Union[Login, None]]:
    auth_method: str = data.get("auth_method")
    user_name_email: Union[str, None] = data.get("user_name_email")
    password: Union[str, None] = data.get("password")
    if auth_method == "SSO":
        login_info: Union[Login, None] = db_handler.get_login(
            auth_method=auth_method, user_name_email=user_name_email
        )
        if login_info is None:
            return (False, None)
    elif auth_method == "USERNAMEEMAIL":
        login_info: Union[Login, None] = db_handler.get_login(
            auth_method=auth_method,
            user_name_email=user_name_email,
        )
        if login_info is None:
            return (False, None)
        password_hash: bytes = hash_password(password, login_info.salt)
        if password_hash != login_info.password_hash:
            return (False, None)
    else:
        return (False, None)
    return (True, login_info)


def hash_password(password: str, salt: bytes) -> bytes:
    return hashlib.scrypt(password.encode(), salt=salt, n=2**14, r=8, p=1)


@jwt.user_identity_loader
def user_identity_lookup(login: Login):
    return login.alt_user_id


@jwt.user_lookup_loader
def user_lookup_callback(_jwt_header, jwt_data) -> Union[Login, None]:
    identity = jwt_data["sub"]
    print(jwt_data)
    return db_handler.get_login(auth_method="ALTUSERID", user_id=identity)


@app.after_request
def refresh_expiring_jwts(response):
    try:
        exp_timestamp = get_jwt()["exp"]
        now = datetime.now(datetime.timezone.utc)
        target_timestamp = datetime.timestamp(now + datetime.timedelta(minutes=30))
        if target_timestamp > exp_timestamp:
            access_token = create_access_token(identity=get_jwt_identity(), fresh=False)
            set_access_cookies(response, access_token)
        return response
    except (RuntimeError, KeyError):
        # Case where there is not a valid JWT. Just return the original response
        return response


@app.route("/login", methods=["POST"])
def login():
    data = request.json
    login_validated, login = validate_login(data)
    if not login_validated:
        return jsonify("Wrong username or password"), 401

    response = jsonify({"msg": "login successful"})
    access_token = create_access_token(identity=login)
    set_access_cookies(response, access_token)

    return response


@app.route("/logout", methods=["POST"])
def logout():
    response = jsonify({"msg": "logout successful"})
    unset_jwt_cookies(response)
    return response


@app.route("/create_account", methods=["POST"])
def create_account():
    data = request.json
    existing_login_user_name: Union[Login, None] = db_handler.get_login(
        auth_method="USERNAMEEMAIL",
        user_name_email=data["user_name"],
    )
    if existing_login_user_name is not None:
        return jsonify({"msg": "username already taken"}), 400

    existing_login_email: Union[Login, None] = db_handler.get_login(
        auth_method="USERNAMEEMAIL",
        user_name_email=data["email"],
    )
    if existing_login_email is not None:
        return jsonify({"msg": "email already in use"}), 400

    salt: bytes = os.urandom(16)
    user_id = uuid.uuid4()
    alt_user_id = uuid.uuid4()
    new_login: Login = Login(
        user_id=user_id,
        alt_user_id=alt_user_id,
        auth_method="PW",
        email=data["email"],
        password_hash=hash_password(data["password"], salt),
        salt=salt,
        user_name=data["user_name"],
        last_login_timestamp=datetime.datetime.now(),
        is_activated=False,
    )
    new_activation: Activation = Activation(
        alt_user_id=alt_user_id, activation_id=uuid.uuid4()
    )
    db_handler.create_login(new_login)
    db_handler.create_activation(new_activation)

    return jsonify({"msg": "user created successfully"}), 200


@app.route("/activate_account/<string:activation_id>", methods=["GET"])
def activate_account(activation_id: str):
    activation: Union[Activation, None] = db_handler.get_activation(
        activation_id=activation_id
    )
    if activation is None:
        return jsonify({"msg": "Account couldn't be activated"}), 400

    existing_login: Union[Login, None] = db_handler.get_login(
        auth_method="ALTUSERID", user_id=activation.alt_user_id
    )
    if existing_login is None:
        return jsonify({"msg": "Account couldn't be activated"}), 400

    db_handler.activate_login(alt_user_id=activation.alt_user_id)
    db_handler.delete_activation(activation.alt_user_id)

    return jsonify({"msg": "Account was activated"})


@app.route("/who_am_i", methods=["GET"])
@jwt_required(fresh=True)
def protected():
    # We can now access our sqlalchemy User object via `current_user`.
    return jsonify(
        alt_user_id=current_user.alt_user_id,
        email=current_user.email,
        username=current_user.user_name,
    )
