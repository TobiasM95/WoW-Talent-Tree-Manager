import { Box, Button, Divider, TextField, Typography } from "@mui/material";
import { Formik } from "formik";
import * as yup from "yup";
import useMediaQuery from "@mui/material/useMediaQuery";
import Header from "../../components/Header";

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
  const isNonMobile = useMediaQuery("(min-width:600px)");

  const handleLoginFormSubmit = (values) => {
    console.log(values);
  };

  const handleCreationFormSubmit = (values) => {
    console.log(values);
  };

  return (
    <Box m="20px">
      <Header title="LOGIN" subtitle="Login via username and password" />

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
                "& > div": { gridColumn: isNonMobile ? undefined : "span 5" },
              }}
            >
              {isNonMobile && <Box gridColumn="span 2" />}
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
                value={values.password}
                name="password"
                error={!!touched.password && !!errors.password}
                helperText={touched.password && errors.password}
                sx={{ gridColumn: "span 1" }}
              />
              {isNonMobile && <Box gridColumn="span 2" />}
            </Box>
            <Box display="flex" justifyContent="center" mt="20px" mb="20px">
              <Button type="submit" color="secondary" variant="contained">
                Login
              </Button>
            </Box>
          </form>
        )}
      </Formik>
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
              <Button type="submit" color="secondary" variant="contained">
                Create account
              </Button>
            </Box>
          </form>
        )}
      </Formik>
    </Box>
  );
};

export default Login;
