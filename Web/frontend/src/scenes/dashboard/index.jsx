import { useState, useEffect } from "react";
import { Button, Typography, Tooltip } from "@mui/material";
import { Box, useTheme } from "@mui/material";
import { useQuery } from "@tanstack/react-query";
import { tokens } from "../../theme";
import Header from "../../components/Header";
import CustomDataGrid from "../../components/CustomDataGrid";
import { classes, classSpecs } from "../../data/classData";
import { buildAPI } from "../../api/buildAPI";

const Dashboard = () => {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);
  const [selectedClass, setSelectedClass] = useState(null);
  const [selectedSpec, setSelectedSpec] = useState(null);
  const [buildData, setBuildData] = useState(null);

  const { isPending, isLoading, isFetching, error, data, refetch } = useQuery({
    queryKey: ["dashboardTopOutlierBuildQuery"],
    queryFn: () => buildAPI.getSpecial(selectedClass, selectedSpec),
    refetchOnWindowFocus: false,
    enabled: false,
    retry: false,
  });

  useEffect(() => {
    setBuildData(null);
  }, [selectedClass]);

  useEffect(() => {
    if (selectedClass && selectedSpec) {
      refetch();
    }
  }, [selectedSpec]);

  useEffect(() => {
    setBuildData(data);
  }, [data]);

  return (
    <Box m="20px">
      <Box display="flex" justifyContent="space-between">
        <Header title="What's best, what's weird, and what's new today!" />
      </Box>

      {/* Grid and charts */}
      <Box display="grid" gridTemplateColumns="repeat(12, 1fr)" gap="20px">
        <Box
          gridColumn="span 6"
          backgroundColor={colors.primary[400]}
          sx={{ borderRadius: "10px" }}
          border={1}
          borderColor={colors.blueAccent[700]}
        >
          <Tooltip
            title={
              <div style={{ whiteSpace: "pre-line" }}>
                {
                  "Best build is the rank 1 build on Warcraftlogs.com on Mythic difficulty.\nWeird build is the build in the top 100 that differs the most from all the other top 100 builds.\nBest build and weird build can be the same if all players run the exact same build or the weird build is rank 1"
                }
              </div>
            }
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
                  fontSize: 14,
                },
              },
            }}
          >
            <Typography variant="h3" textAlign="center" sx={{ my: "5px" }}>
              Best and "weird" specs (?)
            </Typography>
          </Tooltip>
          <Box display="flex" flexShrink="1" justifyContent="center">
            {classes.map((name) => {
              const dimmed = selectedClass && selectedClass !== name;
              const highlighted = selectedClass && selectedClass === name;
              return (
                <Tooltip
                  title={name.charAt(0).toUpperCase() + name.slice(1)}
                  key={`classButton${name}tooltip`}
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
                        fontSize: 14,
                      },
                    },
                  }}
                >
                  <Button
                    key={`classButton${name}`}
                    sx={{
                      minWidth: "16px",
                      maxWidth: "64px",
                      backgroundColor: highlighted ? "#ffffff33" : "none",
                      my: "5px",
                    }}
                    onClick={() => {
                      setSelectedClass(name);
                      setSelectedSpec(null);
                    }}
                  >
                    <img
                      src={`/class_icons/${name}.png`}
                      style={{
                        minWidth: "16px",
                        opacity: dimmed ? "0.4" : "1",
                      }}
                    />
                  </Button>
                </Tooltip>
              );
            })}
          </Box>
          {selectedClass && (
            <Box display="flex" flexShrink="1" justifyContent="center">
              {classSpecs[selectedClass].map((name) => {
                const dimmed = selectedSpec && selectedSpec !== name;
                const highlighted = selectedSpec && selectedSpec === name;
                return (
                  <Tooltip
                    title={name.charAt(0).toUpperCase() + name.slice(1)}
                    key={`specButton${name}tooltip`}
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
                          fontSize: 14,
                        },
                      },
                    }}
                  >
                    <Button
                      key={`specButton${name}`}
                      sx={{
                        minWidth: "16px",
                        maxWidth: "64px",
                        backgroundColor: highlighted ? "#ffffff33" : "none",
                        my: "5px",
                      }}
                      onClick={() => {
                        setSelectedSpec(name);
                      }}
                    >
                      <img
                        src={`/spec_icons/${selectedClass}/${name}.png`}
                        style={{
                          minWidth: "16px",
                          opacity: dimmed ? "0.4" : "1",
                        }}
                      />
                    </Button>
                  </Tooltip>
                );
              })}
            </Box>
          )}
          {selectedSpec &&
            error === null &&
            isPending === false &&
            isFetching === false &&
            buildData && (
              <CustomDataGrid
                columns={buildData.msg["buildColumns"]}
                data={buildData.msg["buildData"]}
                rowIDCol={"actions"}
                height={"60vh"}
              />
            )}
        </Box>
      </Box>
    </Box>
  );
};

export default Dashboard;
