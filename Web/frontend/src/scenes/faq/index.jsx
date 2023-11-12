import { Box, useTheme } from "@mui/material";
import Header from "../../components/Header";
import { tokens } from "../../theme";
import CustomAccordion from "../../components/CustomAccordion";

const FAQ = () => {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);
  return (
    <Box m="20px">
      <Header title="FAQ" subtitle="Frequently Asked Questions Page" />

      <CustomAccordion
        summary={"An Important Question"}
        details={
          "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Suspendisse malesuada lacus ex, sit amet blandit leo lobortis eget."
        }
        theme={theme}
        colors={colors}
      />
      <CustomAccordion
        summary={"Another Important Question"}
        details={
          "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Suspendisse malesuada lacus ex, sit amet blandit leo lobortis eget."
        }
        theme={theme}
        colors={colors}
      />
      <CustomAccordion
        summary={"Your Favorite Question"}
        details={
          "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Suspendisse malesuada lacus ex, sit amet blandit leo lobortis eget."
        }
        theme={theme}
        colors={colors}
      />
      <CustomAccordion
        summary={"Some Random Question"}
        details={
          "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Suspendisse malesuada lacus ex, sit amet blandit leo lobortis eget."
        }
        theme={theme}
        colors={colors}
      />
      <CustomAccordion
        summary={"The Final Question"}
        details={
          "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Suspendisse malesuada lacus ex, sit amet blandit leo lobortis eget."
        }
        theme={theme}
        colors={colors}
      />
    </Box>
  );
};

export default FAQ;
