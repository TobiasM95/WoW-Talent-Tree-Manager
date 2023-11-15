import { Box, IconButton, useTheme, Tooltip } from "@mui/material";
import { DataGrid } from "@mui/x-data-grid";
import { tokens } from "../theme";
import CheckIcon from "@mui/icons-material/Check";
import RemoveRedEyeOutlinedIcon from "@mui/icons-material/RemoveRedEyeOutlined";
import EditOutlinedIcon from "@mui/icons-material/EditOutlined";
import { useNavigate } from "react-router-dom";

const CustomDataGrid = ({ columns, data, rowIDCol, height }) => {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);
  const navigate = useNavigate();

  const viewActionButtonPressed = (content_id) => {
    navigate(`/viewer/${content_id}`);
  };
  const editActionButtonPressed = (content_id) => {
    console.log(content_id);
  };

  if (columns === undefined || data === undefined || rowIDCol === undefined) {
    return <></>;
  }

  columns.forEach((item) => {
    if (Object.hasOwn(item, "convertCheckbox")) {
      item.renderCell = (params) => {
        if (params.row[item.field]) {
          return <CheckIcon />;
        } else {
          return <></>;
        }
      };
    } else if (Object.hasOwn(item, "convertActions")) {
      item.renderCell = ({ row }) => {
        return (
          <Box display="flex" flexDirection={"row"} justifyContent={"center"}>
            <Tooltip
              title="View"
              placement="left"
              slotProps={{
                tooltip: {
                  sx: {
                    bgcolor:
                      theme.palette.mode === "dark"
                        ? `${colors.primary[500]}`
                        : "#fff",
                    "& .MuiTooltip-arrow": {
                      color: "common.black",
                    },
                    border: `1px solid ${colors.grey[100]}`,
                    color: `${colors.grey[100]}`,
                    fontSize: 12,
                  },
                },
              }}
            >
              <IconButton
                onClick={() => {
                  viewActionButtonPressed(row.actions);
                }}
              >
                <RemoveRedEyeOutlinedIcon />
              </IconButton>
            </Tooltip>
            <Tooltip
              title="Edit"
              placement="right"
              slotProps={{
                tooltip: {
                  sx: {
                    bgcolor:
                      theme.palette.mode === "dark"
                        ? `${colors.primary[500]}`
                        : "#fff",
                    "& .MuiTooltip-arrow": {
                      color: "common.black",
                    },
                    border: `1px solid ${colors.grey[100]}`,
                    color: `${colors.grey[100]}`,
                    fontSize: 12,
                  },
                },
              }}
            >
              <IconButton
                onClick={() => {
                  editActionButtonPressed(row.actions);
                }}
              >
                <EditOutlinedIcon />
              </IconButton>
            </Tooltip>
          </Box>
        );
      };
    }
  });

  return (
    <Box height={height ?? "75vh"}>
      <DataGrid
        sx={{
          // "& .MuiDataGrid-cell": { borderBottom: "none" },
          "& .Mui-selected.MuiDataGrid-row": {
            backgroundColor: colors.greenAccent[800],
          },
          "& .Mui-selected.MuiDataGrid-row.Mui-hovered": {
            backgroundColor: colors.greenAccent[700],
          },
          "& .MuiDataGrid-cell:focus": { outline: "none" },
          "& .MuiDataGrid-cell:focus-within": { outline: "none" },
        }}
        getRowId={(row) => row[rowIDCol]}
        rows={data}
        columns={columns}
        density="compact"
        disableRowSelectionOnClick
      />
    </Box>
  );
};

export default CustomDataGrid;
