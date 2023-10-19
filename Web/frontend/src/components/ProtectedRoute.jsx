import { Navigate } from "react-router-dom";
import { useAuth } from "./AuthProvider";

const ProtectedRoute = ({ children }) => {
  const { loginState } = useAuth();

  if (loginState === false) {
    return <Navigate to="/" replace />;
  } else if (loginState === null) {
    return <div></div>;
  }

  return children;
};

export default ProtectedRoute;
