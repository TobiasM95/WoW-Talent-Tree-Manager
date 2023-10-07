import { Box } from "@mui/material";
import { useCallback } from "react";
import { Handle, Position } from "reactflow";
import Image from "mui-image";

function PassiveNode({ data, isConnectable }) {
  const onChange = useCallback((evt) => {
    console.log(evt.target.value);
  }, []);

  return (
    <Box className="passive-nodes" width={80} height={80}>
      <Box overflow="hidden" borderRadius={"50%"} border={4} color="#F00">
        <Image src="../../Icon_bw_variant_1.png" />
      </Box>
      <Box position="absolute" top="70%" left="70%">
        <Image src="../../logo192.png" />
      </Box>
      <Handle
        type="target"
        position={Position.Top}
        isConnectable={isConnectable}
      />
      <Handle
        type="source"
        position={Position.Bottom}
        isConnectable={isConnectable}
      />
    </Box>
  );
}

export default PassiveNode;
