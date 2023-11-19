import os
import json
from dotenv import load_dotenv
import requests
from requests_oauth2client import OAuth2Client, OAuth2ClientCredentialsAuth
from pypika import Query, Table
from components.create_app import db_handler


def main():
    load_dotenv()
    oauth2client, oauth2token, oauth2auth, session = init_oauth2_session()
    top_builds, outlier_builds = get_top_and_outlier_builds(session)
    print(top_builds)
    return


def init_oauth2_session():
    oauth2client = OAuth2Client(
        token_endpoint="https://www.warcraftlogs.com/oauth/token",
        client_id=os.environ["WCL_CLIENT_ID"],
        client_secret=os.environ["WCL_CLIENT_SECRET"],
    )
    token = oauth2client.client_credentials()
    auth = OAuth2ClientCredentialsAuth(oauth2client)
    session = requests.Session()
    session.auth = auth

    return oauth2client, token, auth, session


def get_top_and_outlier_builds(session):
    encounter_ids = json.loads(os.environ["WCL_ENCOUNTER_IDS"])
    encounter_names = json.loads(os.environ["WCL_ENCOUNTER_NAMES"])
    class_spec_combinations = get_class_spec_combinations()
    top_builds = {}
    for encounter_id, encounter_name in zip(encounter_ids, encounter_names):
        top_builds[encounter_id] = {"encounter_name": encounter_name}
        for class_name, spec_name in class_spec_combinations:
            if class_name == "Deathknight":
                class_name = "DeathKnight"
            elif class_name == "Demonhunter":
                class_name == "DemonHunter"

            if class_name not in top_builds[encounter_id]:
                top_builds[encounter_id][class_name] = {}
            print(encounter_id, class_name, spec_name)
            top_builds[encounter_id][class_name][spec_name] = get_top_build(
                session, encounter_id, class_name, spec_name
            )
    return top_builds, None


def get_top_build(session, encounter_id, class_name, spec_name):
    ranking_query = """
query {{
	worldData{{
		encounter(id: {0}){{
			characterRankings(
				difficulty: 5,
				className: "{1}",
				specName: "{2}",
                includeCombatantInfo: true
			)
		}}
	}}
}}
""".format(
        encounter_id, class_name, spec_name
    )
    ranking_json = session.get(
        "https://www.warcraftlogs.com/api/v2/client", json={"query": ranking_query}
    ).json()
    player_ranking = ranking_json["data"]["worldData"]["encounter"][
        "characterRankings"
    ]["rankings"]


def get_class_spec_combinations():
    table = Table("PresetTrees")
    q = Query.from_(table).select("Name")
    class_spec_combinations_raw = db_handler.execute_query(q, expects_results=True)
    class_spec_combinations = [
        raw_combo[0].split(" ")[::-1]
        for raw_combo in class_spec_combinations_raw
        if raw_combo[0] != "New custom Tree"
    ]
    return sorted(class_spec_combinations, key=lambda x: (x[0], x[1]))


if __name__ == "__main__":
    main()
