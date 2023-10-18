import { apiURL } from "../data/settings";

const baseUrl = apiURL;
const loginURL = baseUrl + "/login";
const logoutURL = baseUrl + "/logout";
const loginCheckURL = baseUrl + "/check_if_logged_in";

function translateStatusToErrorMessage(status) {
  return "There was an error retrieving the conversations overview. Please try again.";
}

function checkStatus(response) {
  if (response.ok) {
    return response;
  } else {
    const httpErrorInfo = {
      status: response.status,
      statusText: response.statusText,
      url: response.url,
    };
    console.log(`log server http error: ${JSON.stringify(httpErrorInfo)}`);

    let errorMessage = translateStatusToErrorMessage(httpErrorInfo.status);
    throw new Error(errorMessage);
  }
}

function parseJSON(response) {
  return response.json();
}

function extractLoginStatus(data) {
  return data;
}

const baseAPI = {
  login(loginInformation) {
    const options = {
      method: "POST",
      body: JSON.stringify(loginInformation),
      headers: {
        Accept: "application/json, text/plain",
        "Content-Type": "application/json;charset=UTF-8",
      },
      credentials: "include",
    };
    return fetch(`${loginURL}`, options)
      .then(checkStatus)
      .then(parseJSON)
      .then(extractLoginStatus)
      .catch((error) => {
        console.log("log client error " + error);
        throw new Error("There was a client error during the login process.");
      });
  },
  checkIfLoggedIn() {
    const options = {
      credentials: "include",
    };
    return fetch(`${loginCheckURL}`, options)
      .then(checkStatus)
      .then(parseJSON)
      .then(extractLoginStatus)
      .catch((error) => {
        console.log("log client error " + error);
        throw new Error("There was a client error during the login process.");
      });
  },
  logout() {
    const options = {
      method: "POST",
      credentials: "include",
    };
    return fetch(`${logoutURL}`, options)
      .then(checkStatus)
      .then(parseJSON)
      .then(extractLoginStatus)
      .catch((error) => {
        console.log("log client error " + error);
        throw new Error("There was a client error during the login process.");
      });
  },
};

export { baseAPI };
