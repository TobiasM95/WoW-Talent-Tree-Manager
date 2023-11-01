import os
import datetime

from dotenv import load_dotenv
from flask import Flask
from flask_cors import CORS
from flask_jwt_extended import JWTManager
from sqlalchemy import create_engine
from database_handler import DBHandler

load_dotenv()

db_path = os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    "..",
    "database",
    "data.sqlite",
)
engine = create_engine(f"sqlite+pysqlite:///{db_path}", echo=False)
db_handler: DBHandler = DBHandler(engine)

app: Flask = Flask(__name__)
app.config["JWT_SECRET_KEY"]: str = os.environ["JWT_SECRET_KEY"]
app.config["JWT_TOKEN_LOCATION"] = ["cookies"]
app.config["JWT_COOKIE_SECURE"] = os.environ["JWT_COOKIE_SECURE"] == "True"
app.config["JWT_ACCESS_TOKEN_EXPIRES"] = datetime.timedelta(days=1)
app.config["JWT_COOKIE_SAMESITE"] = "Lax"


CORS(
    app,
    resources={
        r"/*": {
            "origins": [
                "http://localhost:3000",
                "http://127.0.0.1:3000",
                "http://localhost:3000/*",
                "http://127.0.0.1:3000/*",
            ]
        }
    },
    supports_credentials=True,
)

jwt = JWTManager(app)
