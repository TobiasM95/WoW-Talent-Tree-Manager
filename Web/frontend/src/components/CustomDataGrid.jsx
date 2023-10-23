import { Box, useTheme } from "@mui/material";
import { DataGrid } from "@mui/x-data-grid";
import { tokens } from "../theme";
import CheckIcon from "@mui/icons-material/Check";

const CustomDataGrid = ({ columns, data, rowIDCol }) => {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);

  columns.forEach((item) => {
    if (Object.hasOwn(item, "convertCheckbox")) {
      item.renderCell = ({ row: { isImported } }) => {
        if (isImported) {
          return <CheckIcon />;
        } else {
          return <></>;
        }
      };
    }
  });

  return (
    <Box
      height="75vh"
      sx={{
        "& .MuiDataGrid-cell": { borderBottom: "none" },
        "& .Mui-selected.MuiDataGrid-row": {
          backgroundColor: colors.greenAccent[800],
        },
        "& .Mui-selected.MuiDataGrid-row.Mui-hovered": {
          backgroundColor: colors.greenAccent[700],
        },
      }}
    >
      <DataGrid
        getRowId={(row) => row[rowIDCol]}
        rows={data}
        columns={columns}
        density="compact"
      />
    </Box>
  );
};

export default CustomDataGrid;
