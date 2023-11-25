import { apiURL } from "../data/settings";

const baseURL = apiURL;
const buildURL = baseURL + "/build";
const specialBuildURL = buildURL + "/special";

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
    let errorMessage = translateStatusToErrorMessage(httpErrorInfo.status);
    throw new Error(errorMessage);
  }
}

function parseJSON(response) {
  return response.json();
}

const buildAPI = {
  get(content_id) {
    const options = {
      method: "GET",
      credentials: "include",
    };
    return fetch(`${buildURL}/${content_id}`, options)
      .then(checkStatus)
      .then(parseJSON)
      .catch((error) => {
        console.log("log client error " + error);
        throw new Error("There was a client error during the login process.");
      });
  },
  getSpecial(className, specName) {
    const options = {
      method: "GET",
      credentials: "include",
    };
    return fetch(`${specialBuildURL}/${className}/${specName}`, options)
      .then(checkStatus)
      .then(parseJSON)
      .catch((error) => {
        console.log("log client error " + error);
        throw new Error("There was a client error during the login process.");
      });
  },
};

export { buildAPI };
