import { Box, Button, TextField } from "@mui/material";
import { Formik } from "formik";
import * as yup from "yup";
import useMediaQuery from "@mui/material/useMediaQuery";
import Header from "../../components/Header";

const initialValues = {
  userNameEmail: "",
  password: "",
};

const userSchema = yup.object().shape({
  userNameEmail: yup.string().required("required"),
  password: yup.string().required("required"),
});

const Login = () => {
  const isNonMobile = useMediaQuery("(min-width:600px)");

  const handleFormSubmit = (values) => {
    console.log(values);
  };

  return (
    <Box m="20px">
      <Header title="LOGIN" subtitle="Login via password or SSO" />

      <Formik
        onSubmit={handleFormSubmit}
        initialValues={initialValues}
        validationSchema={userSchema}
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
                name="Password"
                error={!!touched.password && !!errors.password}
                helperText={touched.password && errors.password}
                sx={{ gridColumn: "span 1" }}
              />
              {isNonMobile && <Box gridColumn="span 2" />}
            </Box>
            <Box display="flex" justifyContent="center" mt="20px">
              <Button type="submit" color="secondary" variant="contained">
                Login
              </Button>
            </Box>
          </form>
        )}
      </Formik>
    </Box>
  );
};

export default Login;
