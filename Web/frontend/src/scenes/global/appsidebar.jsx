import { useState } from "react";
import { Sidebar, Menu, MenuItem } from "react-pro-sidebar";
import { Box, IconButton, Typography, useTheme } from "@mui/material";
import { NavLink } from "react-router-dom";
import { tokens } from "../../theme";
import HomeOutlinedIcon from "@mui/icons-material/HomeOutlined";
import PeopleOutlinedIcon from "@mui/icons-material/PeopleOutlined";
import PersonOutlinedIcon from "@mui/icons-material/PersonOutlined";
import HelpOutlineOutlinedIcon from "@mui/icons-material/HelpOutlined";
import MenuOutlinedIcon from "@mui/icons-material/MenuOutlined";
import BarChartOutlinedIcon from "@mui/icons-material/BarChartOutlined";

const Item = ({ title, to, icon, selected, setSelected }) => {
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
};

const AppSidebar = () => {
  const theme = useTheme();
  const colors = tokens(theme.palette.mode);
  const [isCollapsed, setIsCollapsed] = useState(false);
  const [selected, setSelected] = useState("Dashboard");

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
    <Box>
      <Sidebar
        collapsed={isCollapsed}
        backgroundColor={`${colors.primary[400]}`}
        style={{ height: "100vh" }}
        rootStyles={{
          borderColor: `transparent`,
        }}
      >
        <div
          style={{ height: "100%", display: "flex", flexDirection: "column" }}
        >
          <div style={{ flex: 1 }}>
            <Menu iconShape="square" menuItemStyles={menuItemStyles}>
              {/* LOGO AND MENU ICON */}
              <MenuItem
                onClick={() => setIsCollapsed(!isCollapsed)}
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
                    <IconButton onClick={() => setIsCollapsed(!isCollapsed)}>
                      <MenuOutlinedIcon />
                    </IconButton>
                  </Box>
                )}
              </MenuItem>
              <Box>
                <Item
                  title="Dashboard"
                  to="/"
                  icon={<HomeOutlinedIcon />}
                  selected={selected}
                  setSelected={setSelected}
                />

                <Typography
                  variant="h6"
                  color={colors.grey[300]}
                  sx={{ m: "15px 0 5px 20px" }}
                >
                  Data
                </Typography>
                <Item
                  title="Manage Team"
                  to="/team"
                  icon={<PeopleOutlinedIcon />}
                  selected={selected}
                  setSelected={setSelected}
                />

                <Typography
                  variant="h6"
                  color={colors.grey[300]}
                  sx={{ m: "15px 0 5px 20px" }}
                >
                  Pages
                </Typography>
                <Item
                  title="Profile Form"
                  to="/form"
                  icon={<PersonOutlinedIcon />}
                  selected={selected}
                  setSelected={setSelected}
                />
                <Item
                  title="FAQ Page"
                  to="/faq"
                  icon={<HelpOutlineOutlinedIcon />}
                  selected={selected}
                  setSelected={setSelected}
                />
                <Typography
                  variant="h6"
                  color={colors.grey[300]}
                  sx={{ m: "15px 0 5px 20px" }}
                >
                  Charts
                </Typography>
                <Item
                  title="Bar Chart"
                  to="/bar"
                  icon={<BarChartOutlinedIcon />}
                  selected={selected}
                  setSelected={setSelected}
                />
                <Item
                  title="Tree View"
                  to="/tree"
                  icon={<BarChartOutlinedIcon />}
                  selected={selected}
                  setSelected={setSelected}
                />
              </Box>
            </Menu>
          </div>
          <div>
            {!isCollapsed && (
              <Box mb="25px">
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
                <Box textAlign="center">
                  <Typography
                    variant="h5"
                    color={colors.grey[100]}
                    sx={{ m: "10px 0 0 0" }}
                  >
                    Github
                  </Typography>
                </Box>
              </Box>
            )}
            {isCollapsed && (
              <Box mb="25px">
                <Box display="flex" justifyContent="center" alignItems="center">
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
    </Box>
  );
};

export default AppSidebar;
