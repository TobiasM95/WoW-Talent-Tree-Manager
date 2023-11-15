import { Typography, Box, useTheme } from "@mui/material";
import { tokens } from "../theme";

import { MuiMarkdown } from "mui-markdown";

const MarkdownTextBox = ({ text }) => {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);
  if (!text || text == "") {
    return <></>;
  }
  return (
    <Box p="10px" border={1}>
      <MuiMarkdown>{text}</MuiMarkdown>
    </Box>
  );
};

export default MarkdownTextBox;
