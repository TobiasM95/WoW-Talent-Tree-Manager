import { ColorModeContext, useMode } from "./theme";
import { CssBaseline, ThemeProvider } from "@mui/material";
import { Routes, Route, Navigate } from "react-router-dom";
import AuthProvider from "./components/AuthProvider";
import { useAuth } from "./components/AuthProvider";
import { baseAPI } from "./api/backendAPI";
import Topbar from "./scenes/global/topbar";
import Dashboard from "./scenes/dashboard";
import AppSidebar from "./scenes/global/appsidebar";
import Team from "./scenes/team";
import Form from "./scenes/form";
import FAQ from "./scenes/faq";
import Bar from "./scenes/bar";
import Tree from "./scenes/tree";
import Login from "./scenes/login";

function App() {
  const [theme, colorMode] = useMode();

  const ProtectedRoute = ({ children }) => {
    const { loginState } = useAuth();

    if (loginState === false) {
      return <Navigate to="/" replace />;
    } else if (loginState === null) {
      return <div></div>;
    }

    return children;
  };

  return (
    <ColorModeContext.Provider value={colorMode}>
      <AuthProvider>
        <ThemeProvider theme={theme}>
          <CssBaseline />
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
        </ThemeProvider>
      </AuthProvider>
    </ColorModeContext.Provider>
  );
}

export default App;
