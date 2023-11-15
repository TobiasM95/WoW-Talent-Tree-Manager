import Accordion from "@mui/material/Accordion";
import AccordionSummary from "@mui/material/AccordionSummary";
import AccordionDetails from "@mui/material/AccordionDetails";
import Typography from "@mui/material/Typography";
import ExpandMoreIcon from "@mui/icons-material/ExpandMore";

const CustomAccordion = ({ summary, content, theme, colors }) => {
  return (
    <Accordion sx={{ border: 1, borderColor: colors.primary[900] }}>
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
        {content}
      </AccordionDetails>
    </Accordion>
  );
};

export default CustomAccordion;
