import {
  Box,
  Typography,
  useTheme,
  Button,
  Divider,
  TextField,
  InputAdornment,
  IconButton,
} from "@mui/material";
import { useParams, useNavigate } from "react-router-dom";
import { useRef } from "react";
import { tokens } from "../../theme";
import SearchIcon from "@mui/icons-material/Search";

const Viewer = () => {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);
  const { contentID } = useParams();
  const contentIDInputRef = useRef("");
  const navigate = useNavigate();

  if (!contentID) {
    return (
      <Box
        height="75vh"
        display="flex"
        flexDirection="column"
        justifyContent="center"
        alignItems="center"
      >
        <Typography variant="h2" fontWeight="bold">
          Select your content
        </Typography>
        <Box
          display="flex"
          flexDirection="row"
          justifyContent="center"
          alignItems="center"
          my="20px"
        >
          <Button
            sx={{ mx: "10px" }}
            variant="outlined"
            onClick={() => {
              navigate("/");
            }}
          >
            <Typography>Popular Content</Typography>
          </Button>
          <Button
            sx={{ mx: "10px" }}
            variant="outlined"
            onClick={() => {
              navigate("/workspace");
            }}
          >
            <Typography>Workspace</Typography>
          </Button>
        </Box>
        <Divider style={{ width: "10%" }}>or</Divider>

        <TextField
          id="contentIDInput"
          label="Content ID"
          inputRef={contentIDInputRef}
          sx={{ my: "20px", width: "300px" }}
          InputProps={{
            endAdornment: (
              <InputAdornment position="end">
                <IconButton
                  aria-label="go"
                  onClick={() => {
                    if (
                      contentIDInputRef.current.value &&
                      contentIDInputRef.current.value.length > 0
                    ) {
                      navigate(`/viewer/${contentIDInputRef.current.value}`);
                    }
                  }}
                  edge="end"
                >
                  <SearchIcon />
                </IconButton>
              </InputAdornment>
            ),
          }}
        />
      </Box>
    );
  }
  return <></>;
};

export default Viewer;
