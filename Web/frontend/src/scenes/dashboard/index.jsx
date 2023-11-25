import { useState, useEffect } from "react";
import { Button, Typography } from "@mui/material";
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

  const { isPending, isLoading, isFetching, error, data, refetch } = useQuery({
    queryKey: ["dashboardTopOutlierBuildQuery"],
    queryFn: () => buildAPI.getSpecial(selectedClass, selectedSpec),
    refetchOnWindowFocus: false,
    enabled: false,
    retry: false,
  });

  useEffect(() => {
    if (selectedClass && selectedSpec) {
      refetch();
    }
  }, [selectedSpec]);

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
          <Typography variant="h3" textAlign="center" sx={{ my: "5px" }}>
            Best and weird specs
          </Typography>
          <Box display="flex" flexShrink="1" justifyContent="center">
            {classes.map((name) => {
              const dimmed = selectedClass && selectedClass !== name;
              const highlighted = selectedClass && selectedClass === name;
              return (
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
              );
            })}
          </Box>
          {selectedClass && (
            <Box display="flex" flexShrink="1" justifyContent="center">
              {classSpecs[selectedClass].map((name) => {
                const dimmed = selectedSpec && selectedSpec !== name;
                const highlighted = selectedSpec && selectedSpec === name;
                return (
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
                );
              })}
            </Box>
          )}
          {selectedSpec &&
            error === null &&
            isPending === false &&
            isFetching === false && (
              <CustomDataGrid
                columns={data.msg["buildColumns"]}
                data={data.msg["buildData"]}
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
