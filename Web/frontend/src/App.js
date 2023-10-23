import { ColorModeContext, useMode } from "./theme";
import { CssBaseline, ThemeProvider } from "@mui/material";
import AppDisplay from "./scenes/global/appdisplay";
import AuthProvider from "./components/AuthProvider";
import { QueryClient, QueryClientProvider } from "@tanstack/react-query";

const queryClient = new QueryClient();

function App() {
  const [theme, colorMode] = useMode();

  return (
    <QueryClientProvider client={queryClient}>
      <ColorModeContext.Provider value={colorMode}>
        <AuthProvider>
          <ThemeProvider theme={theme}>
            <CssBaseline />
            <AppDisplay />
          </ThemeProvider>
        </AuthProvider>
      </ColorModeContext.Provider>
    </QueryClientProvider>
  );
}

export default App;
