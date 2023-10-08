import { ToggleButtonGroup, ToggleButton, useTheme } from "@mui/material";
import { tokens } from "../theme";
import { useState } from "react";

const ToggleButtons = ({ selection }) => {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);

  const [selected, setSelected] = useState(selection[0]);

  const handleSelected = (event, newSelected) => {
    if (newSelected !== null) {
      setSelected(newSelected);
    }
  };

  return (
    <ToggleButtonGroup
      color="primary"
      value={selected}
      exclusive
      onChange={handleSelected}
      aria-label="selection"
    >
      {selection.map((s) => (
        <ToggleButton value={s} aria-label={s} key={s + "_key"}>
          {s}
        </ToggleButton>
      ))}
    </ToggleButtonGroup>
  );
};

export default ToggleButtons;
