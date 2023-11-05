import { ColorModeContext, useMode } from "./theme";
import { CssBaseline, ThemeProvider } from "@mui/material";
import AppDisplay from "./scenes/global/appdisplay";
import AuthProvider from "./components/AuthProvider";
import { QueryClient, QueryClientProvider } from "@tanstack/react-query";
import DragProvider from "./components/DragProvider";

const queryClient = new QueryClient();

function App() {
  const [theme, colorMode] = useMode();

  return (
    <QueryClientProvider client={queryClient}>
      <ColorModeContext.Provider value={colorMode}>
        <AuthProvider>
          <DragProvider>
            <ThemeProvider theme={theme}>
              <CssBaseline />
              <AppDisplay />
            </ThemeProvider>
          </DragProvider>
        </AuthProvider>
      </ColorModeContext.Provider>
    </QueryClientProvider>
  );
}

export default App;
