import { Box, Paper, useTheme } from "@mui/material";
//import { useCallback } from "react";
import { Handle, Position } from "reactflow";
import Image from "mui-image";
import { tokens } from "../../theme";

function PassiveNode({ size = 80 }) {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);

  // const onChange = useCallback((evt) => {
  //   console.log(evt.target.value);
  // }, []);

  return (
    <Box className="passive-nodes" width={size} height={size}>
      <Box overflow="hidden" borderRadius={"50%"} border={4} color="#cca100">
        <Image src="../../Icon_bw_variant_1.png" />
      </Box>
      <Box position="absolute" top="75%" left="65%">
        {/* <Chip label="2 / 2" variant="elevated" color="warning" size="small" /> */}
        <Paper
          variant="outlined"
          square
          sx={{
            width: `${size * 0.55}px`,
            fontSize: `${size * 0.25}px`,
            fontWeight: "600",
            textAlign: "center",
            color: colors.grey[200],
          }}
        >
          2 / 2
        </Paper>
      </Box>
      <Handle type="target" position={Position.Top} isConnectable={true} />
      <Handle type="source" position={Position.Bottom} isConnectable={true} />
    </Box>
  );
}

export default PassiveNode;