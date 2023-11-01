import { useState, createContext, useContext, useEffect } from "react";
import { useNavigate } from "react-router-dom";
import { userAPI } from "../api/userAPI";

export const AuthContext = createContext(null);
export const useAuth = () => {
  return useContext(AuthContext);
};

const AuthProvider = ({ children }) => {
  const navigate = useNavigate();

  const [loginState, setLoginState] = useState(null);
  const [userID, setUserID] = useState(null);

  useEffect(() => {
    checkLogin();
  }, []);

  const checkLogin = async () => {
    var is_logged_in;
    try {
      is_logged_in = await userAPI.checkIfLoggedIn();
    } catch {
      is_logged_in = {
        success: false,
        msg: "There was a client error during the login process or the server cannot be reached.",
      };
    }

    console.log(is_logged_in, loginState);
    if (is_logged_in["success"] === true) {
      setLoginState(true);
      setUserID(is_logged_in["user_id"]);
    } else {
      if (loginState) {
        setLoginState(false);
        setUserID(null);
        navigate("/");
      } else {
        setLoginState(false);
        setUserID(null);
      }
    }
  };

  const setLoginStateManually = (newState) => {
    setLoginState(newState);
  };

  const handleLogin = async (loginInformation) => {
    const login_status = await userAPI.login(loginInformation);
    if (login_status["success"] === true) {
      setLoginState(true);
      setUserID(login_status["user_id"]);
      navigate("/");
    }
    return login_status;
  };

  const handleLogout = async () => {
    const login_status = await userAPI.logout();
    if (login_status["success"] === true) {
      setLoginState(false);
      setUserID(null);
    }
    return login_status;
  };

  const value = {
    loginState,
    userID,
    setLogin: handleLogin,
    setLogout: handleLogout,
    setLoginState: setLoginStateManually,
    checkLogin: checkLogin,
  };

  return <AuthContext.Provider value={value}>{children}</AuthContext.Provider>;
};

export default AuthProvider;
