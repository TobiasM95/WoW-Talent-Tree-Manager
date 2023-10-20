import { Box, Stack, Typography } from "@mui/material";
import { useEffect, useState } from "react";
import { baseAPI } from "../../api/backendAPI";
import { useParams } from "react-router-dom";
import Header from "../../components/Header";
import CheckIcon from "@mui/icons-material/Check";
import ErrorOutlineIcon from "@mui/icons-material/ErrorOutline";

const Activation = () => {
  let { activationID } = useParams();

  const [activated, setActivated] = useState(null);
  const activateAccount = async () => {
    const response_information = await baseAPI.activateAccount(activationID);
    console.log("error after navigate", response_information);
    setActivated(
      response_information["success"] ? response_information["success"] : false
    );
  };

  useEffect(() => {
    activateAccount();
  }, []);

  return (
    <Box m="20px">
      <Header title="ACTIVATION" subtitle="Account activation" />
      <Box
        width="100%"
        height="50vh"
        display="flex"
        justifyContent="center"
        alignItems="center"
      >
        <Stack spacing={2} direction="column" alignItems="center">
          {activated === true ? (
            <CheckIcon />
          ) : activated === false ? (
            <ErrorOutlineIcon />
          ) : null}
          <Typography textAlign="center">
            {activated === null
              ? ""
              : activated === true
              ? "Account activation successful"
              : "Account couldn't be activated"}
          </Typography>
        </Stack>
      </Box>
    </Box>
  );
};

export default Activation;
