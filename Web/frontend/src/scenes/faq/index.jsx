import { Box, useTheme } from "@mui/material";
import Header from "../../components/Header";
import Accordion from "@mui/material/Accordion";
import AccordionSummary from "@mui/material/AccordionSummary";
import AccordionDetails from "@mui/material/AccordionDetails";
import Typography from "@mui/material/Typography";
import ExpandMoreIcon from "@mui/icons-material/ExpandMore";
import { tokens } from "../../theme";

const CustomAccordion = ({ summary, details, theme, colors }) => {
  return (
    <Accordion>
      <AccordionSummary
        expandIcon={<ExpandMoreIcon />}
        sx={{ backgroundColor: colors.primary[400] }}
      >
        <Typography variant="h5">{summary}</Typography>
      </AccordionSummary>
      <AccordionDetails
        sx={{
          backgroundColor:
            theme.palette.mode === "dark"
              ? colors.primary[500]
              : colors.primary[900],
        }}
      >
        <Typography>{details}</Typography>
      </AccordionDetails>
    </Accordion>
  );
};

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
