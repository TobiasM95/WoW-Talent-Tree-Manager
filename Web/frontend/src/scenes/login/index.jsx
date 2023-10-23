import {
  Box,
  Button,
  Divider,
  TextField,
  Typography,
  useTheme,
  Snackbar,
  Alert,
} from "@mui/material";
import { tokens } from "../../theme";
import { Formik } from "formik";
import * as yup from "yup";
import useMediaQuery from "@mui/material/useMediaQuery";
import Header from "../../components/Header";
import { googleClientID } from "../../data/secrets";
import { useAuth } from "../../components/AuthProvider";
import { useRef, useEffect, useState } from "react";
import { userAPI } from "../../api/userAPI";

// LOGIN SCHEMA

const initialLoginValues = {
  userNameEmail: "",
  password: "",
};

const userLoginSchema = yup.object().shape({
  userNameEmail: yup.string().required("required"),
  password: yup.string().required("required"),
});

// CREATION SCHEMA

const initialCreationValues = {
  userNameCreate: "",
  email: "",
  passwordCreate: "",
  passwordConfirmation: "",
};

const userCreationSchema = yup.object().shape({
  userNameCreate: yup.string().required("required"),
  email: yup.string().email().required("required"),
  passwordCreate: yup
    .string()
    .required("required")
    .min(8, "password is too short - should be 8 chars minimum"),
  passwordConfirmation: yup
    .string()
    .oneOf([yup.ref("passwordCreate")], "passwords must match"),
});

const Login = () => {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);
  const isNonMobile = useMediaQuery("(min-width:600px)");

  const [snackbarOpen, setSnackbarOpen] = useState(false);
  const [snackbarText, setSnackbarText] = useState("");
  const [snackbarType, setSnackbarType] = useState("error");

  const setSnackbar = (open, text, type) => {
    setSnackbarOpen(open);
    setSnackbarText(text);
    setSnackbarType(type);
  };

  const handleClose = (event, reason) => {
    if (reason === "clickaway") {
      return;
    }

    setSnackbarOpen(false);
  };

  const { setLogin } = useAuth();

  // GOOGLE SSO
  const divRef = useRef(null);
  useEffect(() => {
    window.google.accounts.id.initialize({
      client_id: googleClientID,
      callback: handleCallbackResponse,
    });
    if (divRef.current) {
      window.google.accounts.id.renderButton(divRef.current, {
        theme: theme.palette.mode === "light" ? "outline" : "filled_black",
        size: "large",
        type: "standard",
        shape: "rectangular",
        text: "signin_with",
      });
    }
  }, [theme.palette.mode]);
  async function handleCallbackResponse(response) {
    const jwt_id_token = response.credential;
    const loginInformation = {
      auth_method: "SSO",
      sso_token: jwt_id_token,
    };
    const error_information = await setLogin(loginInformation);
    // TODO: User error to display an error message
    console.log("error after navigate", error_information);
  }
  /************************/

  const handleLoginFormSubmit = async (values, { resetForm }) => {
    resetForm();
    const loginInformation = {
      auth_method: "USERNAMEEMAIL",
      user_name_email: values["userNameEmail"],
      password: values["password"],
    };
    const error_information = await setLogin(loginInformation);
    console.log("error after navigate", error_information);
    setSnackbar(true, "Wrong username, email or password!", "error");
  };

  const handleCreationFormSubmit = async (values, { resetForm }) => {
    const accountInformation = {
      user_name: values["userNameCreate"],
      email: values["email"],
      password: values["passwordCreate"],
    };
    const response = await userAPI.createAccount(accountInformation);
    console.log("error after navigate", response);
    if (response["success"] === true) {
      setSnackbar(
        true,
        "Account creation successful! Check your email account!",
        "success"
      );
      resetForm();
    } else {
      setSnackbar(true, response["msg"], "error");
    }
  };

  return (
    <Box m="20px">
      <Header title="LOGIN" subtitle="Login via username and password" />
      <Snackbar
        anchorOrigin={{ vertical: "top", horizontal: "center" }}
        autoHideDuration={4000}
        open={snackbarOpen}
        onClose={handleClose}
      >
        <Alert
          onClose={handleClose}
          severity={snackbarType}
          sx={{ width: "100%" }}
        >
          <Typography>{snackbarText}</Typography>
        </Alert>
      </Snackbar>
      <Box
        display="grid"
        gap="30px"
        gridTemplateColumns="repeat(12, minmax(0, 1fr))"
        sx={{
          "& > div": { gridColumn: isNonMobile ? undefined : "span 4" },
        }}
      >
        {isNonMobile && <Box gridColumn="span 2" />}
        <Box sx={{ gridColumn: "span 3" }}>
          <Formik
            onSubmit={handleLoginFormSubmit}
            initialValues={initialLoginValues}
            validationSchema={userLoginSchema}
          >
            {({
              values,
              errors,
              touched,
              handleBlur,
              handleChange,
              handleSubmit,
            }) => (
              <form onSubmit={handleSubmit}>
                <Box
                  display="grid"
                  gap="30px"
                  gridTemplateColumns="repeat(5, minmax(0, 1fr))"
                  sx={{
                    "& > div": {
                      gridColumn: isNonMobile ? undefined : "span 5",
                    },
                  }}
                >
                  <TextField
                    fullWidth
                    variant="filled"
                    type="text"
                    label="Username / Email"
                    onBlur={handleBlur}
                    onChange={handleChange}
                    value={values.userNameEmail}
                    name="userNameEmail"
                    error={!!touched.userNameEmail && !!errors.userNameEmail}
                    helperText={touched.userNameEmail && errors.userNameEmail}
                    sx={{ gridColumn: "span 5" }}
                  />
                  <TextField
                    fullWidth
                    variant="filled"
                    type="password"
                    label="Password"
                    onBlur={handleBlur}
                    onChange={handleChange}
                    value={values.password}
                    name="password"
                    error={!!touched.password && !!errors.password}
                    helperText={touched.password && errors.password}
                    sx={{ gridColumn: "span 5" }}
                  />
                </Box>
                <Box display="flex" justifyContent="center" mt="20px" mb="20px">
                  <Button
                    type="submit"
                    color={
                      snackbarOpen && snackbarType === "error"
                        ? "error"
                        : "secondary"
                    }
                    variant="contained"
                  >
                    Login
                  </Button>
                </Box>
              </form>
            )}
          </Formik>
        </Box>
        <Box
          display="flex"
          alignItems="center"
          justifyContent="center"
          height="100%"
          sx={{ marginX: "50px", gridColumn: "span 2" }}
        >
          <Typography
            color={colors.grey[100]}
            fontWeight="bold"
            textAlign="center"
          >
            - or -
          </Typography>
        </Box>
        <Box
          display="flex"
          alignItems="center"
          justifyContent="center"
          height="100%"
          sx={{ gridColumn: "span 3" }}
        >
          <div ref={divRef}></div>
        </Box>
        {isNonMobile && <Box gridColumn="span 2" />}
      </Box>
      <Divider />
      <Box mt="20px">
        <Header title="CREATE" subtitle="Create an account" />
      </Box>
      <Formik
        onSubmit={handleCreationFormSubmit}
        initialValues={initialCreationValues}
        validationSchema={userCreationSchema}
      >
        {({
          values,
          errors,
          touched,
          handleBlur,
          handleChange,
          handleSubmit,
        }) => (
          <form onSubmit={handleSubmit}>
            <Box
              display="grid"
              gap="30px"
              gridTemplateColumns="repeat(5, minmax(0, 1fr))"
              mt="20px"
              sx={{
                "& > div": { gridColumn: isNonMobile ? undefined : "span 5" },
              }}
            >
              {isNonMobile && <Box gridColumn="span 2" />}
              <TextField
                fullWidth
                variant="filled"
                type="text"
                label="Username"
                onBlur={handleBlur}
                onChange={handleChange}
                value={values.userNameCreate}
                name="userNameCreate"
                error={!!touched.userNameCreate && !!errors.userNameCreate}
                helperText={touched.userNameCreate && errors.userNameCreate}
                sx={{ gridColumn: "span 1" }}
              />
              {isNonMobile && <Box gridColumn="span 2" />}
              {isNonMobile && <Box gridColumn="span 2" />}
              <TextField
                fullWidth
                variant="filled"
                type="text"
                label="Email"
                onBlur={handleBlur}
                onChange={handleChange}
                value={values.email}
                name="email"
                error={!!touched.email && !!errors.email}
                helperText={touched.email && errors.email}
                sx={{ gridColumn: "span 1" }}
              />
              {isNonMobile && <Box gridColumn="span 2" />}
              {isNonMobile && <Box gridColumn="span 2" />}
              <TextField
                fullWidth
                variant="filled"
                type="password"
                label="Password"
                onBlur={handleBlur}
                onChange={handleChange}
                value={values.passwordCreate}
                name="passwordCreate"
                error={!!touched.passwordCreate && !!errors.passwordCreate}
                helperText={touched.passwordCreate && errors.passwordCreate}
                sx={{ gridColumn: "span 1" }}
              />
              {isNonMobile && <Box gridColumn="span 2" />}
              {isNonMobile && <Box gridColumn="span 2" />}
              <TextField
                fullWidth
                variant="filled"
                type="password"
                label="Confirm Password"
                onBlur={handleBlur}
                onChange={handleChange}
                value={values.passwordConfirmation}
                name="passwordConfirmation"
                error={
                  !!touched.passwordConfirmation &&
                  !!errors.passwordConfirmation
                }
                helperText={
                  touched.passwordConfirmation && errors.passwordConfirmation
                }
                sx={{ gridColumn: "span 1" }}
              />
              {isNonMobile && <Box gridColumn="span 2" />}
            </Box>
            <Box display="flex" justifyContent="center" mt="20px">
              <Button
                type="submit"
                color={
                  snackbarOpen && snackbarType === "error"
                    ? "error"
                    : "secondary"
                }
                variant="contained"
              >
                <Typography>Create account</Typography>
              </Button>
            </Box>
          </form>
        )}
      </Formik>
    </Box>
  );
};

export default Login;
