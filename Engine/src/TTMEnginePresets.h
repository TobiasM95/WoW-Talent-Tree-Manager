#pragma once

#include <string>

namespace Presets {//ENGINE PRESETES
    enum CLASS_IDS {
        CLASS_IDS_DRUID = 0,
        CLASS_IDS_EVOKER = 1
    };

    enum DRUID_SPEC_IDS {
        DRUID_SPEC_IDS_RESTORATION = 0
    };

    enum EVOKER_SPEC_IDS {
        EVOKER_SPEC_IDS_DEVASTATION = 0
    };

    //const char* CLASSES[] = { "Death Knight", "Demon Hunter", "Druid", "Evoker", "Hunter", "Hunter", "Mager", "Monk", "Paladin", "Priest", "Rogue", "Shaman", "Warlock", "Warrior"};
    static const char* CLASSES[] = { "Druid" , "Evoker" };
    //const char* SPECS_DRUID[] = {"Feral", "Guardian", "Moonkin", "Restoration"};
    static const char* SPECS_DRUID[] = { "Restoration" };
    static const char* SPECS_EVOKER[] = { "Devastation" };

    const char** RETURN_SPECS(int classID);
    const std::string RETURN_PRESET(int classID, int specID);
    const std::string RETURN_PRESET_BY_NAME(std::string presetNAME);

    static const std::string DRUID_RESTORATION_PRESET =
        "druid_restoration:DebugTree:Test description:Kek loadout:31:3;"
        "0:A1:DA1:0:0:3:1:0::1,2,3;"
        "1:B1:DB1:0:1:2:1:0:0:4;"
        "2:B2:DB2,DB22:1:1:3:2:0:0:5;"
        "3:B3:DB3:1:1:4:1:0:0:6;"
        "4:C1:DC1:0:2:1:1:0:1:7,10;"
        "5:C2:DC2:0:2:3:1:0:2:;"
        "6:C3:DC3:0:2:5:1:0:3:8,9,12;"
        "7:D1:DD1,DD12:1:3:2:2:0:4:11;"
        "8:D2:DD2,DD22:1:3:4:2:0:6:11;"
        "9:D3:DD3VEEEEEEEE EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE EEEEEEEEEEEEEEEEE EEEEEEEEEEE,DD32:1:3:6:2:0:6:;"
        "10:E1:DE1,DE12,DE13:1:4:1:3:0:4:13;"
        "11:E2,SE2:DE2,DSE2:2:4:3:1:0:7,8:14,15;"
        "12:E4:DE4:1:4:5:1:0:6:16;"
        "13:F1:DF1:1:5:1:1:0:10:17,20;"
        "14:F2:DF2,DF22:1:5:2:2:0:11:17;"
        "15:F3:DF3,DF32:1:5:4:2:0:11:18;"
        "16:F4:DF4:1:5:5:1:0:12:19;"
        "17:G1,SG1:DG1,DSG1:2:6:2:1:0:13,14:21;"
        "18:G3:DG3:1:6:4:1:0:15,16:21;"
        "19:G4:DG4,DG42:1:6:5:2:0:16:22;"
        "20:H1,SH1:DH1,DSH1:2:7:1:1:0:13:23,24,25;"
        "21:H3:DH3:1:7:3:1:0:17,18:25,26;"
        "22:H4:DH4:0:7:5:1:0:19:26,27;"
        "23:I1:DI1:1:8:0:1:0:20:28;"
        "24:I2:DI2:1:8:1:1:0:20:;"
        "25:I3:DI3,DI32:1:8:2:2:0:20,21:29;"
        "26:I4:DI4,DI42:1:8:4:2:0:21,22:29;"
        "27:I5:DI5:1:8:6:1:0:22:30;"
        "28:J1,SJ1:DJ1,DSJ1:2:9:0:1:0:23:;"
        "29:J3,SJ1:DJ3,DSJ1:2:9:3:1:0:25,26:;"
        "30:J5,SJ1:DJ5,DSJ1:2:9:6:1:0:27:;"
        "testLoadout:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0;"
        "testLoadout3:1:1:2:1:1:1:1:2:2:2:3:2:1:1:2:2:1:2:1:2:1:1:1:1:1:2:2:1:1:2:1;";

    static const std::string EVOKER_DEVASTATION_PRESET =
        "evoker_devastation:DebugTree:Test description:Kek loadout:31:3;"
        "0:A1:DA1:0:0:3:1:0::1,2,3;"
        "1:B1:DB1:0:1:2:1:0:0:4;"
        "2:B2:DB2,DB22:1:1:3:2:0:0:5;"
        "3:B3:DB3:1:1:4:1:0:0:6;"
        "4:C1:DC1:0:2:1:1:0:1:7,10;"
        "5:C2:DC2:0:2:3:1:0:2:;"
        "6:C3:DC3:0:2:5:1:0:3:8,9,12;"
        "7:D1:DD1,DD12:1:3:2:2:0:4:11;"
        "8:D2:DD2,DD22:1:3:4:2:0:6:11;"
        "9:D3:DD3VEEEEEEEE EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE EEEEEEEEEEEEEEEEE EEEEEEEEEEE,DD32:1:3:6:2:0:6:;"
        "10:E1:DE1,DE12,DE13:1:4:1:3:0:4:13;"
        "11:E2,SE2:DE2,DSE2:2:4:3:1:0:7,8:14,15;"
        "12:E4:DE4:1:4:5:1:0:6:16;"
        "13:F1:DF1:1:5:1:1:0:10:17,20;"
        "14:F2:DF2,DF22:1:5:2:2:0:11:17;"
        "15:F3:DF3,DF32:1:5:4:2:0:11:18;"
        "16:F4:DF4:1:5:5:1:0:12:19;"
        "17:G1,SG1:DG1,DSG1:2:6:2:1:0:13,14:21;"
        "18:G3:DG3:1:6:4:1:0:15,16:21;"
        "19:G4:DG4,DG42:1:6:5:2:0:16:22;"
        "20:H1,SH1:DH1,DSH1:2:7:1:1:0:13:23,24,25;"
        "21:H3:DH3:1:7:3:1:0:17,18:25,26;"
        "22:H4:DH4:0:7:5:1:0:19:26,27;"
        "23:I1:DI1:1:8:0:1:0:20:28;"
        "24:I2:DI2:1:8:1:1:0:20:;"
        "25:I3:DI3,DI32:1:8:2:2:0:20,21:29;"
        "26:I4:DI4,DI42:1:8:4:2:0:21,22:29;"
        "27:I5:DI5:1:8:6:1:0:22:30;"
        "28:J1,SJ1:DJ1,DSJ1:2:9:0:1:0:23:;"
        "29:J3,SJ1:DJ3,DSJ1:2:9:3:1:0:25,26:;"
        "30:J5,SJ1:DJ5,DSJ1:2:9:6:1:0:27:;"
        "testLoadout:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0;";
}