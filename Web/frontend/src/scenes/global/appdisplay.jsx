import { useState, useEffect } from "react";
import { Routes, Route } from "react-router-dom";
import { useAuth } from "../../components/AuthProvider";
import { Box, CircularProgress, Typography, Stack } from "@mui/material";
import ProtectedRoute from "../../components/ProtectedRoute";
import Topbar from "../global/topbar";
import Dashboard from "../dashboard";
import AppSidebar from "../global/appsidebar";
import Team from "../team";
import Form from "../form";
import FAQ from "../faq";
import Bar from "../bar";
import Tree from "../tree";
import Login from "../login";

const AppDisplay = () => {
  const { loginState } = useAuth();
  const [canShow, setCanShow] = useState(loginState !== null);

  // Set Time out
  useEffect(() => {
    const timer = setTimeout(
      () => setCanShow(true),
      loginState === null ? 1000 : 0
    );
    return () => clearTimeout(timer);
  }, [loginState]);
  if (!canShow) {
    return <></>;
  }
  if (loginState === null) {
    return (
      <Box
        width="100vw"
        height="100vh"
        display="flex"
        justifyContent="center"
        alignItems="center"
      >
        <Stack spacing={2} direction="column" alignItems="center">
          <CircularProgress />
          <Typography textAlign="center">Loading TTM...</Typography>
        </Stack>
      </Box>
    );
  }
  return (
    <div className="app">
      <AppSidebar />
      <main className="content">
        <Topbar />
        <Routes>
          <Route path="/" element={<Dashboard />} />
          <Route
            path="/team"
            element={
              <ProtectedRoute>
                <Team />
              </ProtectedRoute>
            }
          />
          <Route path="/form" element={<Form />} />
          <Route path="/faq" element={<FAQ />} />
          <Route path="/bar" element={<Bar />} />
          <Route path="/tree" element={<Tree />} />
          <Route path="/login" element={<Login />} />
        </Routes>
      </main>
    </div>
  );
};

export default AppDisplay;
