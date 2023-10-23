import { apiURL } from "../data/settings";

const baseUrl = apiURL;
const workspaceURL = baseUrl + "/workspace";

function translateStatusToErrorMessage(status) {
  return "There was an error. Please try again.";
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
    if (httpErrorInfo.status === 401) {
      return response;
    }
    let errorMessage = translateStatusToErrorMessage(httpErrorInfo.status);
    throw new Error(errorMessage);
  }
}

function parseJSON(response) {
  return response.json();
}

const workspaceAPI = {
  get() {
    const options = {
      method: "GET",
      credentials: "include",
    };
    return fetch(`${workspaceURL}`, options)
      .then(checkStatus)
      .then(parseJSON)
      .catch((error) => {
        console.log("log client error " + error);
        throw new Error("There was a client error during the login process.");
      });
  },
};

export { workspaceAPI };
