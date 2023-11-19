import os
from dotenv import load_dotenv
import requests
from requests_oauth2client import OAuth2Client, OAuth2ClientCredentialsAuth


def main():
    load_dotenv()
    oauth2client, oauth2token, oauth2auth, session = init_oauth2_session()
    print(oauth2token)
    test_body = """
query {
	worldData{
		encounter(id: 2820){
			characterRankings(
				difficulty: 5,
				className: "Druid",
				specName: "Feral",
			)
		}
	}
}
"""
    res = session.get(
        "https://www.warcraftlogs.com/api/v2/client", json={"query": test_body}
    )


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


if __name__ == "__main__":
    main()
