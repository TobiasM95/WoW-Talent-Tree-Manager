import { useState } from "react";
import { Sidebar, Menu, MenuItem } from "react-pro-sidebar";
import { Box, IconButton, Typography, useTheme, Tooltip } from "@mui/material";
import { NavLink } from "react-router-dom";
import { useAuth } from "../../components/AuthProvider";
import { tokens } from "../../theme";
import PersonOutlinedIcon from "@mui/icons-material/PersonOutlined";
import HelpOutlineOutlinedIcon from "@mui/icons-material/HelpOutlined";
import MenuOutlinedIcon from "@mui/icons-material/MenuOutlined";
import BarChartOutlinedIcon from "@mui/icons-material/BarChartOutlined";
import AccountTreeIcon from "@mui/icons-material/AccountTree";
import ViewComfyOutlinedIcon from "@mui/icons-material/ViewComfyOutlined";
import FolderOutlinedIcon from "@mui/icons-material/FolderOutlined";
import EditOutlinedIcon from "@mui/icons-material/EditOutlined";
import QueryStatsOutlinedIcon from "@mui/icons-material/QueryStatsOutlined";
import AssessmentOutlinedIcon from "@mui/icons-material/AssessmentOutlined";

const Item = ({ title, to, icon, selected, setSelected, isCollapsed }) => {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);
  if (isCollapsed === true) {
    return (
      <Tooltip
        title={title}
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
        {/* span is here to avoid error with tooltip and child ref */}
        <span>
          <MenuItem
            active={selected === title}
            onClick={() => setSelected(title)}
            icon={icon}
            component={<NavLink to={to} />}
          >
            <Typography>{title}</Typography>
          </MenuItem>
        </span>
      </Tooltip>
    );
  } else {
    return (
      <MenuItem
        active={selected === title}
        onClick={() => setSelected(title)}
        icon={icon}
        component={<NavLink to={to} />}
      >
        <Typography>{title}</Typography>
      </MenuItem>
    );
  }
};

const AppSidebar = () => {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);
  var ls = require("local-storage");

  const [isCollapsed, setIsCollapsed] = useState(
    ls.get("sidebarCollapsed") ? ls.get("sidebarCollapsed") : false
  );
  const [selected, setSelected] = useState("Dashboard");

  const { loginState } = useAuth();

  const toggleCollapse = () => {
    setIsCollapsed(!isCollapsed);
    ls.set("sidebarCollapsed", !isCollapsed);
  };

  const menuItemStyles = {
    button: {
      "&:hover": {
        backgroundColor: `${colors.primary[900]}`,
        color: `${colors.grey[100]}`,
      },
      [`&.active`]: {
        backgroundColor: `${colors.primary[700]}`,
        color: `${colors.primary[100]}`,
      },
    },
  };

  return (
    <Sidebar
      collapsed={isCollapsed}
      backgroundColor={`${colors.primary[400]}`}
      rootStyles={{
        borderColor: `transparent`,
      }}
      style={{
        height: "100vh",
      }}
    >
      <div style={{ height: "100%", display: "flex", flexDirection: "column" }}>
        <div style={{ flex: 1 }}>
          <Menu iconShape="square" menuItemStyles={menuItemStyles}>
            {/* LOGO AND MENU ICON */}
            <MenuItem
              onClick={toggleCollapse}
              icon={isCollapsed ? <MenuOutlinedIcon /> : undefined}
              style={{
                margin: "10px 0 20px 0",
                color: colors.grey[100],
              }}
            >
              {!isCollapsed && (
                <Box
                  display="flex"
                  justifyContent="space-between"
                  alignItems="center"
                  ml="15px"
                >
                  <Typography color={colors.grey[100]}>
                    Talent Tree Manager
                  </Typography>
                  <IconButton onClick={toggleCollapse}>
                    <MenuOutlinedIcon />
                  </IconButton>
                </Box>
              )}
            </MenuItem>
            <Box>
              <Item
                title="Popular"
                to="/"
                icon={<AssessmentOutlinedIcon />}
                selected={selected}
                setSelected={setSelected}
                isCollapsed={isCollapsed}
              />
              <Item
                title="Configurator"
                to="/configurator"
                icon={<AccountTreeIcon />}
                selected={selected}
                setSelected={setSelected}
                isCollapsed={isCollapsed}
              />

              {loginState === true && (
                <Box>
                  <Typography
                    variant="h6"
                    color={colors.grey[300]}
                    sx={{ m: "15px 0 5px 20px" }}
                  >
                    Member
                  </Typography>
                  <Item
                    title="Workspace"
                    to="/workspace"
                    icon={<FolderOutlinedIcon />}
                    selected={selected}
                    setSelected={setSelected}
                    isCollapsed={isCollapsed}
                  />
                  <Item
                    title="Viewer"
                    to="/viewer"
                    icon={<ViewComfyOutlinedIcon />}
                    selected={selected}
                    setSelected={setSelected}
                    isCollapsed={isCollapsed}
                  />
                  <Item
                    title="Editor"
                    to="/editor"
                    icon={<EditOutlinedIcon />}
                    selected={selected}
                    setSelected={setSelected}
                    isCollapsed={isCollapsed}
                  />
                  <Item
                    title="Analysis"
                    to="/analysis"
                    icon={<QueryStatsOutlinedIcon />}
                    selected={selected}
                    setSelected={setSelected}
                    isCollapsed={isCollapsed}
                  />
                </Box>
              )}
            </Box>
          </Menu>
        </div>
        <div>
          {!isCollapsed && (
            <Box my="25px">
              <Box display="flex" justifyContent="center" alignItems="center">
                <a
                  href="https://github.com/TobiasM95/WoW-Talent-Tree-Manager"
                  target="_blank"
                  rel="noreferrer"
                >
                  <img
                    alt="profile-user"
                    width="100px"
                    height="100px"
                    src={`../../Icon_bw_variant_1.png`}
                    style={{ cursor: "pointer", borderRadius: "50%" }}
                  />
                </a>
              </Box>
              <Box
                textAlign="center"
                justifyContent="center"
                alignItems="center"
                display="flex"
              >
                <Typography
                  variant="h5"
                  color={colors.grey[100]}
                  sx={{ m: "10px 0 0 0" }}
                >
                  Github
                </Typography>
              </Box>
              <Box
                textAlign="center"
                justifyContent="center"
                alignItems="center"
                display="flex"
              >
                <iframe
                  src="https://ghbtns.com/github-btn.html?user=TobiasM95&repo=WoW-Talent-Tree-Manager&type=star&count=true&size=small"
                  frameBorder="0"
                  width="78"
                  height="30"
                  title="TTM"
                ></iframe>
              </Box>
            </Box>
          )}
          {isCollapsed && (
            <Box mb="25px">
              <Box
                display="flex"
                flexDirection="column"
                justifyContent="center"
                alignItems="center"
              >
                <a
                  href="https://github.com/TobiasM95/WoW-Talent-Tree-Manager"
                  target="_blank"
                  rel="noreferrer"
                >
                  <img
                    alt="profile-user"
                    width="25px"
                    height="25px"
                    src={`../../Icon_bw_variant_1.png`}
                    style={{ cursor: "pointer", borderRadius: "50%" }}
                  />
                </a>
              </Box>
            </Box>
          )}
        </div>
      </div>
    </Sidebar>
  );
};

export default AppSidebar;
