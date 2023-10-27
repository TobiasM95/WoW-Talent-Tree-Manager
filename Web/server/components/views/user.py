from components.create_app import app, db_handler, jwt

import os
import hashlib
import datetime
import uuid
from typing import Union
from google.oauth2 import id_token
from google.auth.transport import requests

from flask import request, jsonify
from flask_jwt_extended import (
    create_access_token,
    get_jwt_identity,
    jwt_required,
    current_user,
    set_access_cookies,
    unset_jwt_cookies,
    get_jwt,
)
from database_handler import Login, Activation
from email_handler import EmailHandler


def validate_login(data: dict) -> tuple[bool, Union[Login, None]]:
    auth_method: str = data.get("auth_method")
    user_name_email: Union[str, None] = data.get("user_name_email")
    if user_name_email is not None:
        user_name_email = user_name_email.lower()
    password: Union[str, None] = data.get("password")
    sso_token: Union[str, None] = data.get("sso_token")
    if auth_method == "SSO":
        try:
            idinfo = id_token.verify_oauth2_token(
                sso_token, requests.Request(), os.environ["GOOGLE_AUTH_CLIENT_ID"]
            )
            print(idinfo)

            # ID token is valid. Get the user's Google Account ID from the decoded token.
            user_name_email = idinfo["email"]
        except ValueError:
            # Invalid token
            return (False, None)
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
    return (login_info.is_activated, login_info)


def hash_password(password: str, salt: bytes) -> bytes:
    return hashlib.scrypt(password.encode(), salt=salt, n=2**14, r=8, p=1)


@jwt.user_identity_loader
def user_identity_lookup(login: Login):
    return login.alt_user_id


@jwt.user_lookup_loader
def user_lookup_callback(_jwt_header, jwt_data) -> Union[Login, None]:
    identity = jwt_data["sub"]
    return db_handler.get_login(auth_method="ALTUSERID", user_id=identity)


@app.after_request
def refresh_expiring_jwts(response):
    try:
        exp_timestamp = get_jwt()["exp"]
        now = datetime.datetime.now(datetime.timezone.utc)
        target_timestamp = datetime.datetime.timestamp(
            now + datetime.timedelta(minutes=30)
        )
        if target_timestamp > exp_timestamp:
            access_token = create_access_token(identity=get_jwt_identity(), fresh=False)
            set_access_cookies(response, access_token)
        return response
    except (RuntimeError, KeyError):
        # Case where there is not a valid JWT. Just return the original response
        return response


@app.route("/login", methods=["POST"])
def login():
    data: dict = request.json
    login_validated, login_information = validate_login(data)
    if not login_validated:
        if login_information is None:
            return jsonify({"success": False, "msg": "Wrong username or password"}), 200
        else:
            return jsonify({"success": False, "msg": "Account is not activated"}), 200

    response = jsonify(
        {
            "success": True,
            "msg": "login successful",
            "user_id": login_information.user_id,
        }
    )
    access_token = create_access_token(identity=login_information, fresh=True)
    set_access_cookies(response, access_token)

    return response, 200


@app.route("/logout", methods=["POST"])
def logout():
    response = jsonify({"success": True, "msg": "logout successful"})
    unset_jwt_cookies(response)
    return response


@app.route("/create_account", methods=["POST"])
def create_account():
    data = request.json
    print(data)
    existing_login_user_name: Union[Login, None] = db_handler.get_login(
        auth_method="USERNAMEEMAIL",
        user_name_email=data["user_name"].lower(),
    )
    if existing_login_user_name is not None:
        return jsonify({"success": False, "msg": "Username already taken!"}), 200

    existing_login_email: Union[Login, None] = db_handler.get_login(
        auth_method="USERNAMEEMAIL",
        user_name_email=data["email"],
    )
    if existing_login_email is not None:
        return jsonify({"success": False, "msg": "Email already in use!"}), 200

    salt: bytes = os.urandom(16)
    user_id = str(uuid.uuid4())
    alt_user_id = str(uuid.uuid4())
    new_login: Login = Login(
        user_id=user_id,
        alt_user_id=alt_user_id,
        auth_method="PW",
        email=data["email"],
        password_hash=hash_password(data["password"], salt),
        salt=salt,
        user_name=data["user_name"].lower(),
        last_login_timestamp=datetime.datetime.now(),
        is_activated=False,
    )
    new_activation: Activation = Activation(
        alt_user_id=alt_user_id, activation_id=str(uuid.uuid4())
    )
    db_handler.create_login(new_login)
    db_handler.create_activation(new_activation)
    EmailHandler.send_verification_email(data["email"], new_activation.activation_id)

    return jsonify({"success": True, "msg": "user created successfully"}), 200


@app.route("/activate_account/<string:activation_id>", methods=["GET"])
def activate_account(activation_id: str):
    activation: Union[Activation, None] = db_handler.get_activation(
        activation_id=activation_id
    )
    if activation is None:
        return jsonify({"success": False, "msg": "Account couldn't be activated"}), 200

    existing_login: Union[Login, None] = db_handler.get_login(
        auth_method="ALTUSERID", user_id=activation.alt_user_id
    )
    if existing_login is None:
        return jsonify({"success": False, "msg": "Account couldn't be activated"}), 200

    db_handler.activate_login(alt_user_id=activation.alt_user_id)
    db_handler.delete_activation(activation.alt_user_id)

    return jsonify({"success": True, "msg": "Account was activated"})


@app.route("/delete_account", methods=["GET"])
@jwt_required(fresh=True)
def delete_account():
    # We can now access our sqlalchemy User object via `current_user`.
    db_handler.delete_login(current_user.user_id)
    response = jsonify({"success": True, "msg": "Account deleted successfully"})
    unset_jwt_cookies(response)
    return response


@app.route("/check_if_logged_in", methods=["GET"])
@jwt_required(optional=True)
def check_if_logged_in():
    current_identity = get_jwt_identity()
    if current_identity:
        return (
            jsonify(
                {"success": True, "msg": "Logged in", "user_id": current_user.user_id}
            ),
            200,
        )
    else:
        return jsonify({"success": False, "msg": "Not logged in"}), 200
