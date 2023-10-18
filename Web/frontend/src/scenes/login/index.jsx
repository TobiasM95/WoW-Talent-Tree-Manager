import {
  Box,
  Button,
  Divider,
  TextField,
  Typography,
  useTheme,
} from "@mui/material";
import { tokens } from "../../theme";
import { Formik } from "formik";
import * as yup from "yup";
import useMediaQuery from "@mui/material/useMediaQuery";
import Header from "../../components/Header";
import { googleClientID } from "../../data/secrets";
import { useAuth } from "../../components/AuthProvider";
import { useRef, useEffect } from "react";
import { baseAPI } from "../../api/backendAPI";

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
  userName: "",
  email: "",
  initialPassword: "",
};

const userCreationSchema = yup.object().shape({
  userName: yup.string().required("required"),
  email: yup.string().email().required("required"),
  initialPassword: yup.string().required("required"),
});

const Login = () => {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);
  const isNonMobile = useMediaQuery("(min-width:600px)");

  const { loginState, setLogin } = useAuth();

  const manualLoginCheck = () => {
    async function checkLogin() {
      const is_logged_in = await baseAPI.checkIfLoggedIn();
      console.log(is_logged_in, loginState);
    }
    checkLogin();
  };

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

  const handleLoginFormSubmit = async (values) => {
    const loginInformation = {
      auth_method: "USERNAMEEMAIL",
      user_name_email: values["userNameEmail"],
      password: values["password"],
    };
    const error_information = await setLogin(loginInformation);
    // TODO: User error to display an error message
    console.log("error after navigate", error_information);
  };

  const handleCreationFormSubmit = (values) => {
    console.log(values);
  };

  return (
    <Box m="20px">
      <Header title="LOGIN" subtitle="Login via username and password" />

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
                    type="text"
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
                  <Button type="submit" color="secondary" variant="contained">
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
                value={values.userName}
                name="userName"
                error={!!touched.userName && !!errors.userName}
                helperText={touched.userName && errors.userName}
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
                type="text"
                label="Password"
                onBlur={handleBlur}
                onChange={handleChange}
                value={values.initialPassword}
                name="initialPassword"
                error={!!touched.initialPassword && !!errors.initialPassword}
                helperText={touched.initialPassword && errors.initialPassword}
                sx={{ gridColumn: "span 1" }}
              />
              {isNonMobile && <Box gridColumn="span 2" />}
            </Box>
            <Box display="flex" justifyContent="center" mt="20px">
              <Button
                sx={{
                  backgroundColor: colors.greenAccent[500],
                  fontSize: "14px",
                  fontWeight: "bold",
                  padding: "10px 20px",
                  mb: "20px",
                }}
                type="submit"
                color="secondary"
                variant="contained"
              >
                <Typography>Create account</Typography>
              </Button>
            </Box>
          </form>
        )}
      </Formik>
      <Divider></Divider>
      <Box display="flex" justifyContent="center" mt="20px">
        <Button
          sx={{
            backgroundColor: colors.greenAccent[500],
            fontSize: "14px",
            fontWeight: "bold",
            padding: "10px 20px",
          }}
          type="submit"
          color="secondary"
          variant="contained"
          onClick={manualLoginCheck}
        >
          <Typography>Check if logged in</Typography>
        </Button>
      </Box>
    </Box>
  );
};

export default Login;
