#pragma once

#include <string>
#include <vector>

namespace Engine {
    template<class T>
    using vec2d = std::vector<std::vector<T>>;

    //skillset index is an unsigned 64 bit integer for now
    using SIND = std::uint64_t;
}

namespace Presets {//ENGINE PRESETS
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

    static const std::string NEW_CUSTOM_TREE_PRESET =
        "custom:New custom Tree:"
        "Welcome to WoW Talent Tree Manager.\nThis is a solution to manage trees and their loadouts. A loadout is a collection of skillsets, \ne.g. a skillset for Raid/M+/PvP etc. You can edit trees or work with spec presets, manage \nvarious skillsets in your loadout as well as auto solve all possible skill combinations with a \ngiven filter.\n\nEdit the tree name/description here. To edit tree nodes select \"Tree Editor\" in the top right.\nPress \"Save/Load Trees\" to load tree spec presets, manage your custom trees and import or\nexport trees (from/to Discord, etc.).\nTo start the tree editing process, go to \"Tree Editor\" -> \"Create Node\" and create your first talent.\n\nSelect the \"Talent Loadout Editor\" in the top left to manage your loadout. You can \nedit the loadout description there and create/import/export skillsets. Since skillsets are stored\ninside the loadout which is part of the tree, you can save your skillsets by saving the tree\nin the tree editor.\n\nLastly, select the \"Talent Loadout Solver\" to generate all possible combinations of talent\nselections for all possible amounts of spendable talent points. Afterwards, you can filter\nthe results to include/exclude specific talents and load the results into your loadout.\n\nHint__/__ This text can be edited!:"
        "Your loadout is a collection of different skillset that you can edit to suit various ingame situations,\ne.g. a raid setup, an M+ setup or different PvP skillsets.\nAll skillsets are stored in the loadout which in turn is stored in the tree. So if you save your tree\nyou'll save your loadout as well! Additionally, you can import/export skillsets directly, to share\nwith friends or your favorite discord class experts.:"
        "0:0;";

    /*
    First line (";" is line separator, ":" separates different parts of a single line, "," separates individual components of a property): 
        1. custom or spec preset, if spec preset then spec name all lowercase, only letters, spaces transformed to "_" (just like in simc)
            a. if preset then all the talent point definitions can be omitted if preset is stored in a library (I build this library up with a custom string but that's implementation detail)
        2. Tree name
        3. Tree description
        4. Loadout description (a loadout is a list of skill sets, e.g. "single target", "aoe", that get defined at the end)
        5. number of talents in the tree
        6. number of loadouts

    Then N lines for N talents:
        1. Index (doesn't have to be starting from 0 and in order but loadouts define talent points spent for each talent ordered by index so it's easier to keep it that way)
        2. Talent name (does not have to be "A1".. this is the real talent name, if switch talent then we have 2 names separated by ",")
        3. Talent descriptions (1 description for each maxRank, or 2 if switch talent, separated by ",")
        4. Talent type (0 is active, 1 is passive, 2 is "switch")
        5. Row position
        6. Column position
        7. Max points
        8. Points required
        9. Parent indices
        10. Child indices

    Then N lines for N loadouts:
        1. Loadout name
        2. Points spent in talent (max is maxRank or 2 for switch talent. switch talent is either 0 for not selected, 1 for first, 2 for second)
            a. The points are in order for ordered talents (ordered by index)
    */
    static const std::string DRUID_RESTORATION_PRESET =
        "druid_restoration:DebugTree:Test description:Kek loadout:31:3;"
        "0:A1:DA1__/__Testcolon:0:1:4:1:0::1,2,3;"
        "1:B1:DB1:0:2:3:1:0:0:4;"
        "2:B2:DB2,DB22:1:2:4:2:0:0:5;"
        "3:B3:DB3:1:2:5:1:0:0:6;"
        "4:C1:DC1:0:3:2:1:0:1:7,10;"
        "5:C2:DC2:0:3:4:1:0:2:;"
        "6:C3:DC3:0:3:6:1:0:3:8,9,12;"
        "7:D1:DD1,DD12:1:4:3:2:0:4:11;"
        "8:D2:DD2,DD22:1:4:5:2:0:6:11;"
        "9:D3:DD3VEEEEEEEE EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE EEEEEEEEEEEEEEEEE EEEEEEEEEEE,DD32:1:4:7:2:0:6:;"
        "10:E1:DE1,DE12,DE13:1:5:2:3:0:4:13;"
        "11:E2,SE2:DE2,DSE2:2:5:4:1:0:7,8:14,15;"
        "12:E4:DE4:1:5:6:1:0:6:16;"
        "13:F1:DF1:1:6:2:1:0:10:17,20;"
        "14:F2:DF2,DF22:1:6:3:2:0:11:17;"
        "15:F3:DF3,DF32:1:6:5:2:0:11:18;"
        "16:F4:DF4:1:6:6:1:0:12:18,19;"
        "17:G1,SG1:DG1,DSG1:2:7:3:1:0:13,14:21;"
        "18:G3:DG3:1:7:5:1:0:15,16:21;"
        "19:G4:DG4,DG42:1:7:6:2:0:16:22;"
        "20:H1,SH1:DH1,DSH1:2:8:2:1:0:13:23,24,25;"
        "21:H3:DH3:1:8:4:1:0:17,18:25,26;"
        "22:H4:DH4:0:8:6:1:0:19:26,27;"
        "23:I1:DI1:1:9:1:1:0:20:28;"
        "24:I2:DI2:1:9:2:1:0:20:;"
        "25:I3:DI3,DI32:1:9:3:2:0:20,21:29;"
        "26:I4:DI4,DI42:1:9:5:2:0:21,22:29;"
        "27:I5:DI5:1:9:7:1:0:22:30;"
        "28:J1,SJ1:DJ1,DSJ1:2:10:1:1:0:23:;"
        "29:J3,SJ1:DJ3,DSJ1:2:10:4:1:0:25,26:;"
        "30:J5,SJ1:DJ5,DSJ1:2:10:7:1:0:27:;"
        "testLoadout:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0;"
        "testLoadout3:1:1:2:1:1:1:1:2:2:2:3:2:1:1:2:2:1:2:1:2:1:1:1:1:1:2:2:1:1:2:1;";

    static const std::string EVOKER_DEVASTATION_PRESET =
        "evoker_devastation:DebugTree:Test description:Kek loadout:31:3;"
        "0:A1:DA1__/__Testcolon:0:1:4:1:0::1,2,3;"
        "1:B1:DB1:0:2:3:1:0:0:4;"
        "2:B2:DB2,DB22:1:2:4:2:0:0:5;"
        "3:B3:DB3:1:2:5:1:0:0:6;"
        "4:C1:DC1:0:3:2:1:0:1:7,10;"
        "5:C2:DC2:0:3:4:1:0:2:;"
        "6:C3:DC3:0:3:6:1:0:3:8,9,12;"
        "7:D1:DD1,DD12:1:4:3:2:0:4:11;"
        "8:D2:DD2,DD22:1:4:5:2:0:6:11;"
        "9:D3:DD3VEEEEEEEE EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE EEEEEEEEEEEEEEEEE EEEEEEEEEEE,DD32:1:4:7:2:0:6:;"
        "10:E1:DE1,DE12,DE13:1:5:2:3:0:4:13;"
        "11:E2,SE2:DE2,DSE2:2:5:4:1:0:7,8:14,15;"
        "12:E4:DE4:1:5:6:1:0:6:16;"
        "13:F1:DF1:1:6:2:1:0:10:17,20;"
        "14:F2:DF2,DF22:1:6:3:2:0:11:17;"
        "15:F3:DF3,DF32:1:6:5:2:0:11:18;"
        "16:F4:DF4:1:6:6:1:0:12:18,19;"
        "17:G1,SG1:DG1,DSG1:2:7:3:1:0:13,14:21;"
        "18:G3:DG3:1:7:5:1:0:15,16:21;"
        "19:G4:DG4,DG42:1:7:6:2:0:16:22;"
        "20:H1,SH1:DH1,DSH1:2:8:2:1:0:13:23,24,25;"
        "21:H3:DH3:1:8:4:1:0:17,18:25,26;"
        "22:H4:DH4:0:8:6:1:0:19:26,27;"
        "23:I1:DI1:1:9:1:1:0:20:28;"
        "24:I2:DI2:1:9:2:1:0:20:;"
        "25:I3:DI3,DI32:1:9:3:2:0:20,21:29;"
        "26:I4:DI4,DI42:1:9:5:2:0:21,22:29;"
        "27:I5:DI5:1:9:7:1:0:22:30;"
        "28:J1,SJ1:DJ1,DSJ1:2:10:1:1:0:23:;"
        "29:J3,SJ1:DJ3,DSJ1:2:10:4:1:0:25,26:;"
        "30:J5,SJ1:DJ5,DSJ1:2:10:7:1:0:27:;"
        "testLoadout:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0:0;";
}