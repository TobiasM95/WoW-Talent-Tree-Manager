/*
Content API combines tree, loadout and build api to fetch all the components
that connect to the content id, i.e. tree for a tree content id,
loadut + tree for a loadout content id and build + loadout + tree for a 
build content id
*/

import { apiURL } from "../data/settings";

const baseUrl = apiURL;
const contentURL = baseUrl + "/content";

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

const contentAPI = {
  get() {
    const options = {
      method: "GET",
      credentials: "include",
    };
    return fetch(`${contentURL}`, options)
      .then(checkStatus)
      .then(parseJSON)
      .catch((error) => {
        console.log("log client error " + error);
        throw new Error("There was a client error during the login process.");
      });
  },
};

export { contentAPI };
