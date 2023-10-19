import { ColorModeContext, useMode } from "./theme";
import { CssBaseline, ThemeProvider } from "@mui/material";
import AppDisplay from "./scenes/global/appdisplay";
import AuthProvider from "./components/AuthProvider";

function App() {
  const [theme, colorMode] = useMode();

  return (
    <ColorModeContext.Provider value={colorMode}>
      <AuthProvider>
        <ThemeProvider theme={theme}>
          <CssBaseline />
          <AppDisplay />
        </ThemeProvider>
      </AuthProvider>
    </ColorModeContext.Provider>
  );
}

export default App;
