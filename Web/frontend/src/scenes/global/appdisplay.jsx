import { useState, useEffect } from "react";
import { Routes, Route, useLocation } from "react-router-dom";
import { useAuth } from "../../components/AuthProvider";
import { Box, CircularProgress, Typography, Stack } from "@mui/material";
import ProtectedRoute from "../../components/ProtectedRoute";
import Topbar from "../global/topbar";
import Dashboard from "../dashboard";
import Configurator from "../configurator";
import Workspace from "../workspace";
import Viewer from "../viewer";
import Editor from "../editor";
import Analysis from "../analysis";
import AppSidebar from "../global/appsidebar";
import Login from "../login";
import Activation from "../activation";

const AppDisplay = () => {
  const { loginState, checkLogin } = useAuth();
  const [canShow, setCanShow] = useState(loginState !== null);

  let location = useLocation();

  useEffect(() => {
    const timer = setTimeout(
      () => setCanShow(true),
      loginState === null ? 1000 : 0
    );
    return () => clearTimeout(timer);
  }, [loginState]);

  useEffect(() => {
    checkLogin();
  }, [location]);

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
          <Route path="/configurator" element={<Configurator />} />
          <Route
            path="/workspace"
            element={
              <ProtectedRoute>
                <Workspace />
              </ProtectedRoute>
            }
          />
          <Route
            path="/viewer/:contentID?"
            element={
              <ProtectedRoute>
                <Viewer />
              </ProtectedRoute>
            }
          />
          <Route
            path="/editor"
            element={
              <ProtectedRoute>
                <Editor></Editor>
              </ProtectedRoute>
            }
          />
          <Route
            path="/analysis"
            element={
              <ProtectedRoute>
                <Analysis></Analysis>
              </ProtectedRoute>
            }
          />
          <Route path="/login" element={<Login />} />
          <Route path="/activation/:activationID?" element={<Activation />} />
        </Routes>
      </main>
    </div>
  );
};

export default AppDisplay;
