import os
import json
import uuid
from dotenv import load_dotenv
import requests
import concurrent.futures
from requests_oauth2client import OAuth2Client, OAuth2ClientCredentialsAuth
from pypika import Query, Table
from components.create_app import db_handler
from database_handler import PresetBuild
import pandas as pd
import numpy as np


def create_popular_builds():
    load_dotenv()
    oauth2client, oauth2token, oauth2auth, session = init_oauth2_session()
    top_builds, outlier_builds = get_top_and_outlier_builds(session)
    save_top_and_outlier_builds(top_builds, outlier_builds)


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
    outlier_builds = {}
    talent_id_to_node_id_dict = get_talent_id_to_node_id_dict()

    table = Table("PresetTalents")
    talent_query = Query.from_(table).select("ContentID", "NodeID", "OrderID")
    talent_data = db_handler.execute_query(talent_query, expects_results=True)
    df_talent_data = pd.DataFrame(
        data=talent_data, columns=["ContentID", "NodeID", "OrderID"]
    )
    node_id_order_id_dict = {
        row.NodeID: row.OrderID for _, row in df_talent_data.iterrows()
    }

    with concurrent.futures.ThreadPoolExecutor(max_workers=11) as executor:
        build_futures = {}
        for encounter_id, encounter_name in list(zip(encounter_ids, encounter_names)):
            top_builds[encounter_id] = {"encounter_name": encounter_name}
            outlier_builds[encounter_id] = {"encounter_name": encounter_name}
            for class_name, spec_name in class_spec_combinations:
                if class_name == "Deathknight":
                    class_name = "DeathKnight"
                elif class_name == "Demonhunter":
                    class_name = "DemonHunter"
                elif spec_name == "Beastmastery":
                    spec_name = "BeastMastery"

                if class_name not in top_builds[encounter_id]:
                    top_builds[encounter_id][class_name] = {}

                if class_name not in outlier_builds[encounter_id]:
                    outlier_builds[encounter_id][class_name] = {}
                build_futures[
                    executor.submit(
                        get_top_and_outlier_build,
                        session,
                        encounter_id,
                        class_name,
                        spec_name,
                        talent_id_to_node_id_dict,
                        df_talent_data,
                        node_id_order_id_dict,
                    )
                ] = [encounter_id, class_name, spec_name]

        for future in concurrent.futures.as_completed(build_futures):
            encounter_id, class_name, spec_name = build_futures[future]
            try:
                top_build, outlier_build = future.result()
                top_builds[encounter_id][class_name][spec_name] = top_build
                outlier_builds[encounter_id][class_name][spec_name] = outlier_build
                print(encounter_id, class_name, spec_name, "successful")
            except Exception as e:
                print(
                    encounter_id,
                    class_name,
                    spec_name,
                    "generated an exception",
                    str(e),
                )
    return top_builds, outlier_builds


def get_top_and_outlier_build(
    session,
    encounter_id,
    class_name,
    spec_name,
    talent_id_to_node_id_dict,
    df_talent_data,
    node_id_order_id_dict,
):
    top_100_node_ids = get_top_100_node_ids(
        session, encounter_id, class_name, spec_name, talent_id_to_node_id_dict
    )
    df_class_talents, df_spec_talents = get_empty_talent_dfs(
        class_name, spec_name, len(top_100_node_ids), df_talent_data
    )
    df_class_talents, df_spec_talents = fill_dfs(
        df_class_talents, df_spec_talents, top_100_node_ids
    )
    top_build = [
        {
            str(node_id_order_id_dict[node_id]): points
            for node_id, points in df_class_talents.iloc[0, :].items()
            if points > 0
        },
        {
            str(node_id_order_id_dict[node_id]): points
            for node_id, points in df_spec_talents.iloc[0, :].items()
            if points > 0
        },
    ]
    outlier_index = np.argmin(
        (df_spec_talents.to_numpy() * df_spec_talents.mean(axis=0).to_numpy()).sum(
            axis=1
        )
    )
    outlier_build = [
        {
            str(node_id_order_id_dict[node_id]): points
            for node_id, points in df_class_talents.iloc[outlier_index, :].items()
            if points > 0
        },
        {
            str(node_id_order_id_dict[node_id]): points
            for node_id, points in df_spec_talents.iloc[outlier_index, :].items()
            if points > 0
        },
    ]
    return top_build, outlier_build


def get_top_100_node_ids(
    session, encounter_id, class_name, spec_name, talent_id_to_node_id_dict
):
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
    try:
        player_rankings = ranking_json["data"]["worldData"]["encounter"][
            "characterRankings"
        ]["rankings"]
    except Exception as e:
        # fmt: off
        # from IPython import embed; embed(); input("Enter to continue...")
        # fmt: on
        raise e
    top_100_node_ids = []
    for player_ranking in player_rankings:
        top_100_node_ids.append(
            {
                # see comment in get_talent_id_to_node_id_dict() function for info
                # about the final indices 0 and 1
                talent_id_to_node_id_dict[talent["talentID"]][0]: (
                    talent["points"] + talent_id_to_node_id_dict[talent["talentID"]][1]
                )
                for talent in player_ranking["talents"]
            }
        )
    return top_100_node_ids


def get_talent_id_to_node_id_dict():
    tree_info_json = requests.get(
        "https://www.raidbots.com/static/data/live/talents.json"
    ).json()
    talent_id_to_node_id_dict = {}
    # use enumerate to get the index value of the "entries" field
    # active and passive talents have only one entry and i is 0
    # switch talents have 2 entries and the second switch talent get's i = 1
    # we add this later to the points spent to assign 2 if the second switch talent
    # is takent and 0 otherwise and don't change active and passive counts at all
    for class_spec_info in tree_info_json:
        for class_node in class_spec_info["classNodes"]:
            for i, entry in enumerate(class_node["entries"]):
                talent_id_to_node_id_dict[entry["id"]] = [class_node["id"], i]
        for spec_node in class_spec_info["specNodes"]:
            for i, entry in enumerate(spec_node["entries"]):
                talent_id_to_node_id_dict[entry["id"]] = [spec_node["id"], i]
    return talent_id_to_node_id_dict


def get_empty_talent_dfs(class_name, spec_name, num_rows, df_talent_data):
    table = Table("PresetTrees")
    tree_query = (
        Query.from_(table)
        .where(table.Name == (spec_name.capitalize() + " " + class_name.capitalize()))
        .select("ClassTalents", "SpecTalents")
    )
    content_ids = db_handler.execute_query(tree_query, expects_results=True)[0]
    class_talent_content_ids = json.loads(content_ids[0])
    spec_talent_content_ids = json.loads(content_ids[1])
    df_class_talent_content_ids = pd.DataFrame(
        data=class_talent_content_ids, columns=["ContentID"]
    )
    df_spec_talent_content_ids = pd.DataFrame(
        data=spec_talent_content_ids, columns=["ContentID"]
    )

    df_class_talent_content_ids = df_class_talent_content_ids.merge(
        df_talent_data, on="ContentID", how="left"
    )
    df_spec_talent_content_ids = df_spec_talent_content_ids.merge(
        df_talent_data, on="ContentID", how="left"
    )
    df_class_empty = pd.DataFrame(
        np.zeros((num_rows, df_class_talent_content_ids.shape[0]), dtype=int),
        columns=sorted(df_class_talent_content_ids.NodeID.to_list()),
    )
    df_spec_empty = pd.DataFrame(
        np.zeros((num_rows, df_spec_talent_content_ids.shape[0]), dtype=int),
        columns=sorted(df_spec_talent_content_ids.NodeID.to_list()),
    )
    return df_class_empty, df_spec_empty


def fill_dfs(df_class_talents, df_spec_talents, top_100_node_ids):
    for i, node_ids in enumerate(top_100_node_ids):
        for node_id, points in node_ids.items():
            if node_id in df_class_talents.columns:
                df_class_talents.loc[i, node_id] = points
            elif node_id in df_spec_talents.columns:
                df_spec_talents.loc[i, node_id] = points
            else:
                # fmt: off
                # from IPython import embed; embed(); input("Enter to continue...")
                # fmt: on
                raise Exception(f"Node id {node_id} does not exist")
    return df_class_talents, df_spec_talents


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


def save_top_and_outlier_builds(top_builds, outlier_builds, recreate=False):
    if recreate:
        recreate_top_and_outlier_builds(top_builds, outlier_builds)
    else:
        update_top_and_outlier_builds(top_builds, outlier_builds)


def update_top_and_outlier_builds(top_builds, outlier_builds):
    for encounter_id, classes_dict in top_builds.items():
        for class_name, spec_dict in classes_dict.items():
            if class_name == "encounter_name":
                continue
            for spec_name, assigned_points in spec_dict.items():
                tree_table = Table("PresetTrees")
                tree_id_query = (
                    Query.from_(tree_table)
                    .select("ContentID")
                    .where(
                        tree_table.Name
                        == f"{spec_name.capitalize()} {class_name.capitalize()}"
                    )
                )
                res = db_handler.execute_query(tree_id_query, True)
                tree_id = res[0][0]
                preset_build = PresetBuild(
                    str(uuid.uuid4()),
                    encounter_id,
                    tree_id,
                    None,
                    f"{classes_dict['encounter_name']} top {spec_name.capitalize()} {class_name.capitalize()} build",
                    class_name.capitalize(),
                    spec_name.capitalize(),
                    70,
                    True,
                    json.dumps(assigned_points),
                    f"This is the top performing {spec_name.capitalize()} {class_name.capitalize()} build for the \"{classes_dict['encounter_name']}\" encounter.",
                )
                db_handler.update_preset_build(preset_build, "TopBuilds")

    for encounter_id, classes_dict in outlier_builds.items():
        for class_name, spec_dict in classes_dict.items():
            if class_name == "encounter_name":
                continue
            for spec_name, assigned_points in spec_dict.items():
                tree_table = Table("PresetTrees")
                tree_id_query = (
                    Query.from_(tree_table)
                    .select("ContentID")
                    .where(
                        tree_table.Name
                        == f"{spec_name.capitalize()} {class_name.capitalize()}"
                    )
                )
                res = db_handler.execute_query(tree_id_query, True)
                tree_id = res[0][0]
                preset_build = PresetBuild(
                    str(uuid.uuid4()),
                    encounter_id,
                    tree_id,
                    None,
                    f"{classes_dict['encounter_name']} outlier {spec_name.capitalize()} {class_name.capitalize()} build",
                    class_name.capitalize(),
                    spec_name.capitalize(),
                    70,
                    True,
                    json.dumps(assigned_points),
                    f"This is the outlier {spec_name.capitalize()} {class_name.capitalize()} build for the \"{classes_dict['encounter_name']}\" encounter.",
                )
                db_handler.update_preset_build(preset_build, "OutlierBuilds")


def recreate_top_and_outlier_builds(top_builds, outlier_builds):
    db_handler.execute_query(Query.from_("OutlierBuilds").delete())
    db_handler.execute_query(Query.from_("TopBuilds").delete())
    for encounter_id, classes_dict in top_builds.items():
        for class_name, spec_dict in classes_dict.items():
            if class_name == "encounter_name":
                continue
            for spec_name, assigned_points in spec_dict.items():
                tree_table = Table("PresetTrees")
                tree_id_query = (
                    Query.from_(tree_table)
                    .select("ContentID")
                    .where(
                        tree_table.Name
                        == f"{spec_name.capitalize()} {class_name.capitalize()}"
                    )
                )
                res = db_handler.execute_query(tree_id_query, True)
                tree_id = res[0][0]
                preset_build = PresetBuild(
                    str(uuid.uuid4()),
                    encounter_id,
                    tree_id,
                    None,
                    f"{classes_dict['encounter_name']} top {spec_name.capitalize()} {class_name.capitalize()} build",
                    class_name.capitalize(),
                    spec_name.capitalize(),
                    70,
                    True,
                    json.dumps(assigned_points),
                    f"This is the top performing {spec_name.capitalize()} {class_name.capitalize()} build for the \"{classes_dict['encounter_name']}\" encounter.",
                )
                db_handler.create_preset_build(preset_build, "TopBuilds")

    for encounter_id, classes_dict in outlier_builds.items():
        for class_name, spec_dict in classes_dict.items():
            if class_name == "encounter_name":
                continue
            for spec_name, assigned_points in spec_dict.items():
                tree_table = Table("PresetTrees")
                tree_id_query = (
                    Query.from_(tree_table)
                    .select("ContentID")
                    .where(
                        tree_table.Name
                        == f"{spec_name.capitalize()} {class_name.capitalize()}"
                    )
                )
                res = db_handler.execute_query(tree_id_query, True)
                tree_id = res[0][0]
                preset_build = PresetBuild(
                    str(uuid.uuid4()),
                    encounter_id,
                    tree_id,
                    None,
                    f"{classes_dict['encounter_name']} outlier {spec_name.capitalize()} {class_name.capitalize()} build",
                    class_name.capitalize(),
                    spec_name.capitalize(),
                    70,
                    True,
                    json.dumps(assigned_points),
                    f"This is the outlier {spec_name.capitalize()} {class_name.capitalize()} build for the \"{classes_dict['encounter_name']}\" encounter.",
                )
                db_handler.create_preset_build(preset_build, "OutlierBuilds")


if __name__ == "__main__":
    create_popular_builds()
