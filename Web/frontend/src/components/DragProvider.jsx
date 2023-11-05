import { useState, createContext, useContext, useEffect } from "react";

export const DragContext = createContext(null);
export const useDrag = () => {
  return useContext(DragContext);
};

const DragProvider = ({ children }) => {
  const [isDragging, setIsDragging] = useState(false);
  // we need this because reactflow calls onmove once after it's done rendering
  // probably to position the tree initially
  const [isReadyToDrag, setIsReadyToDrag] = useState(false);

  const handleSetIsDragging = (value) => {
    if (!value) {
      setIsDragging(false);
    } else if (isReadyToDrag) {
      setIsDragging(true);
    }
  };

  const handleSetDragReady = (value) => {
    setIsReadyToDrag(value);
  };

  const value = {
    isDragging,
    isReadyToDrag,
    setIsDragging: handleSetIsDragging,
    setDragReady: handleSetDragReady,
  };

  return <DragContext.Provider value={value}>{children}</DragContext.Provider>;
};

export default DragProvider;
