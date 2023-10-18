import { useState, createContext, useContext, useEffect } from "react";
import { useNavigate } from "react-router-dom";
import { baseAPI } from "../api/backendAPI";

export const AuthContext = createContext(null);
export const useAuth = () => {
  return useContext(AuthContext);
};

const AuthProvider = ({ children }) => {
  const navigate = useNavigate();

  const [loginState, setLoginState] = useState(null);

  useEffect(() => {
    async function checkLogin() {
      const is_logged_in = await baseAPI.checkIfLoggedIn();
      console.log(is_logged_in, loginState);
      setLoginState(is_logged_in["success"]);
    }
    checkLogin();
  }, []);

  const setLoginStateManually = (newState) => {
    setLoginState(newState);
  };

  const handleLogin = async (loginInformation) => {
    const login_status = await baseAPI.login(loginInformation);
    if (login_status["success"] === true) {
      setLoginState(true);
      navigate("/");
    }
    return login_status;
  };

  const handleLogout = async () => {
    const login_status = await baseAPI.logout();
    if (login_status["success"] === true) {
      setLoginState(false);
    }
    return login_status;
  };

  const value = {
    loginState,
    setLogin: handleLogin,
    setLogout: handleLogout,
    setLoginState: setLoginStateManually,
  };

  return <AuthContext.Provider value={value}>{children}</AuthContext.Provider>;
};

export default AuthProvider;
