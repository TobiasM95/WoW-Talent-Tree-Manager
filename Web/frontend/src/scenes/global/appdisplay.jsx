import { useState, useEffect } from "react";
import { Routes, Route } from "react-router-dom";
import { useAuth } from "../../components/AuthProvider";
import { Box, CircularProgress, Typography, Stack } from "@mui/material";
import ProtectedRoute from "../../components/ProtectedRoute";
import Topbar from "../global/topbar";
import Dashboard from "../dashboard";
import Workspace from "../workspace";
import AppSidebar from "../global/appsidebar";
import Team from "../team";
import Form from "../form";
import FAQ from "../faq";
import Bar from "../bar";
import Tree from "../tree";
import Login from "../login";
import Activation from "../activation";

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
          <Route path="/configurator" element={<></>} />
          <Route
            path="/workspace"
            element={
              <ProtectedRoute>
                <Workspace />
              </ProtectedRoute>
            }
          />
          <Route
            path="/viewer"
            element={
              <ProtectedRoute>
                <></>
              </ProtectedRoute>
            }
          />
          <Route
            path="/editor"
            element={
              <ProtectedRoute>
                <></>
              </ProtectedRoute>
            }
          />
          <Route
            path="/analysis"
            element={
              <ProtectedRoute>
                <></>
              </ProtectedRoute>
            }
          />
          <Route path="/team" element={<Team />} />
          <Route path="/form" element={<Form />} />
          <Route path="/faq" element={<FAQ />} />
          <Route path="/bar" element={<Bar />} />
          <Route path="/tree" element={<Tree />} />
          <Route path="/login" element={<Login />} />
          <Route path="/activation/:activationID?" element={<Activation />} />
        </Routes>
      </main>
    </div>
  );
};

export default AppDisplay;