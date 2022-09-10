#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <map>

namespace Engine {
    template<class T>
    using vec2d = std::vector<std::vector<T>>;

    //skillset index is an unsigned 64 bit integer for now
    using SIND = std::uint64_t;
}

namespace Presets {//ENGINE PRESETS
    static const std::string TTM_VERSION = "1.3.8";

    enum CLASS_IDS {
        CLASS_IDS_DEATHKNIGHT = 0,
        CLASS_IDS_DEMONHUNTER = 1,
        CLASS_IDS_DRUID = 2,
        CLASS_IDS_EVOKER = 3,
        CLASS_IDS_HUNTER = 4,
        CLASS_IDS_MAGE = 5,
        CLASS_IDS_MONK = 6,
        CLASS_IDS_PALADIN = 7,
        CLASS_IDS_PRIEST = 8,
        CLASS_IDS_ROGUE = 9,
        CLASS_IDS_SHAMAN = 10,
        CLASS_IDS_WARLOCK = 11,
        CLASS_IDS_WARRIOR = 12
    };

    enum DEATHKNIGHT_SPEC_IDS {
        DEATHKNIGHT_SPEC_IDS_BLOOD = 0,
        DEATHKNIGHT_SPEC_IDS_FROST = 1,
        DEATHKNIGHT_SPEC_IDS_UNHOLY = 2,
        DEATHKNIGHT_SPEC_IDS_CLASS_BLOOD = 3,
        DEATHKNIGHT_SPEC_IDS_CLASS_FROST = 4,
        DEATHKNIGHT_SPEC_IDS_CLASS_UNHOLY = 5
    };

    enum DEMONHUNTER_SPEC_IDS {
        DEMONHUNTER_SPEC_IDS_HAVOC = 0,
        DEMONHUNTER_SPEC_IDS_VENGEANCE = 1,
        DEMONHUNTER_SPEC_IDS_CLASS_HAVOC = 2,
        DEMONHUNTER_SPEC_IDS_CLASS_VENGEANCE = 3
    };

    enum DRUID_SPEC_IDS {
        DRUID_SPEC_IDS_BALANCE = 0,
        DRUID_SPEC_IDS_FERAL = 1,
        DRUID_SPEC_IDS_GUARDIAN = 2,
        DRUID_SPEC_IDS_RESTORATION = 3,
        DRUID_SPEC_IDS_CLASS_BALANCE = 4,
        DRUID_SPEC_IDS_CLASS_FERAL = 5,
        DRUID_SPEC_IDS_CLASS_GUARDIAN = 6,
        DRUID_SPEC_IDS_CLASS_RESTORATION = 7
    };

    enum EVOKER_SPEC_IDS {
        EVOKER_SPEC_IDS_DEVASTATION = 0,
        EVOKER_SPEC_IDS_PRESERVATION = 1,
        EVOKER_SPEC_IDS_CLASS_DEVASTATION = 2,
        EVOKER_SPEC_IDS_CLASS_PRESERVATION = 3
    };

    enum HUNTER_SPEC_IDS {
        HUNTER_SPEC_IDS_BEASTMASTERY = 0,
        HUNTER_SPEC_IDS_MARKSMANSHIP = 1,
        HUNTER_SPEC_IDS_SURVIVAL = 2,
        HUNTER_SPEC_IDS_CLASS_BEASTMASTERY = 3,
        HUNTER_SPEC_IDS_CLASS_MARKSMANSHIP = 4,
        HUNTER_SPEC_IDS_CLASS_SURVIVAL = 5
    };

    enum MAGE_SPEC_IDS {
        MAGE_SPEC_IDS_ARCANE = 0,
        MAGE_SPEC_IDS_FIRE = 1,
        MAGE_SPEC_IDS_FROST = 2,
        MAGE_SPEC_IDS_CLASS_ARCANCE = 3,
        MAGE_SPEC_IDS_CLASS_FIRE = 4,
        MAGE_SPEC_IDS_CLASS_FROST = 5
    };

    enum MONK_SPEC_IDS {
        MONK_SPEC_IDS_BREWMASTER = 0,
        MONK_SPEC_IDS_MISTWEAVER = 1,
        MONK_SPEC_IDS_WINDWALKER = 2,
        MONK_SPEC_IDS_CLASS_BREWMASTER = 3,
        MONK_SPEC_IDS_CLASS_MISTWEAVER = 4,
        MONK_SPEC_IDS_CLASS_WINDWALKER = 5
    };

    enum PALADIN_SPEC_IDS {
        PALADIN_SPEC_IDS_HOLY = 0,
        PALADIN_SPEC_IDS_PROTECTION = 1,
        PALADIN_SPEC_IDS_RETRIBUTION = 2,
        PALADIN_SPEC_IDS_CLASS_HOLY = 3,
        PALADIN_SPEC_IDS_CLASS_PROTECTION = 4,
        PALADIN_SPEC_IDS_CLASS_RETRIBUTION = 5
    };

    enum PRIEST_SPEC_IDS {
        PRIEST_SPEC_IDS_DISCIPLINE = 0,
        PRIEST_SPEC_IDS_HOLY = 1,
        PRIEST_SPEC_IDS_SHADOW = 2,
        PRIEST_SPEC_IDS_CLASS_DISCIPLINE = 3,
        PRIEST_SPEC_IDS_CLASS_HOLY = 4,
        PRIEST_SPEC_IDS_CLASS_SHADOW = 5
    };

    enum ROGUE_SPEC_IDS {
        ROGUE_SPEC_IDS_ASSASSINATION = 0,
        ROGUE_SPEC_IDS_OUTLAW = 1,
        ROGUE_SPEC_IDS_SUBTLETY = 2,
        ROGUE_SPEC_IDS_CLASS_ASSASSINATION = 3,
        ROGUE_SPEC_IDS_CLASS_OUTLAW = 4,
        ROGUE_SPEC_IDS_CLASS_SUBTLETY = 5
    };

    enum SHAMAN_SPEC_IDS {
        SHAMAN_SPEC_IDS_ELEMENTAL = 0,
        SHAMAN_SPEC_IDS_ENHANCEMENT = 1,
        SHAMAN_SPEC_IDS_RESTORATION = 2,
        SHAMAN_SPEC_IDS_CLASS_ELEMENTAL = 3,
        SHAMAN_SPEC_IDS_CLASS_ENHANCEMENT = 4,
        SHAMAN_SPEC_IDS_CLASS_RESTORATION = 5
    };

    enum WARLOCK_SPEC_IDS {
        WARLOCK_SPEC_IDS_AFFLICTION = 0,
        WARLOCK_SPEC_IDS_DEMONOLOGY = 1,
        WARLOCK_SPEC_IDS_DESTRUCTION = 2,
        WARLOCK_SPEC_IDS_CLASS_AFFLICTION = 3,
        WARLOCK_SPEC_IDS_CLASS_DEMONOLOGY = 4,
        WARLOCK_SPEC_IDS_CLASS_DESTRUCTION = 5
    };

    enum WARRIOR_SPEC_IDS {
        WARRIOR_SPEC_IDS_ARMS = 0,
        WARRIOR_SPEC_IDS_FURY = 1,
        WARRIOR_SPEC_IDS_PROTECTION = 2,
        WARRIOR_SPEC_IDS_CLASS_ARMS = 3,
        WARRIOR_SPEC_IDS_CLASS_FURY = 4,
        WARRIOR_SPEC_IDS_CLASS_PROTECTION = 5
    };

    static const char* CLASSES[] = { "Death Knight", "Demon Hunter", "Druid", "Evoker", "Hunter", "Mage", "Monk", "Paladin", "Priest", "Rogue", "Shaman", "Warlock", "Warrior"};
    static const char* SPECS_DEATHKNIGHT[] = { "Blood", "Frost", "Unholy" , "Class tree (Blood)" , "Class tree (Frost)" , "Class tree (Unholy)" };
    static const char* SPECS_DEMONHUNTER[] = { "Havoc", "Vengeance" , "Class tree (Havoc)", "Class tree (Vengeance)"};
    static const char* SPECS_DRUID[] = {"Balance", "Feral", "Guardian", "Restoration", "Class tree (Balance)", "Class tree (Feral)", "Class tree (Guardian)", "Class tree (Restoration)" };
    static const char* SPECS_EVOKER[] = { "Devastation" , "Preservation", "Class tree (Devastation)" , "Class tree (Preservation)" };
    static const char* SPECS_HUNTER[] = { "Beast Mastery", "Marksmanship", "Survival" , "Class tree (Beast Mastery)", "Class tree (Marksmanship)", "Class tree (Survival)" };
    static const char* SPECS_MAGE[] = { "Arcane", "Fire", "Frost" , "Class tree (Arcance)", "Class tree (Fire)", "Class tree (Frost)" };
    static const char* SPECS_MONK[] = { "Brewmaster", "Mistweaver", "Windwalker" , "Class tree (Brewmaster)", "Class tree (Mistweaver)", "Class tree (Windwalker)" };
    static const char* SPECS_PALADIN[] = { "Holy", "Protection", "Retribution" , "Class tree (Holy)", "Class tree (Protection)", "Class tree (Retribution)" };
    static const char* SPECS_PRIEST[] = { "Discipline", "Holy", "Shadow" , "Class tree (Discipline)", "Class tree (Holy)", "Class tree (Shadow)" };
    static const char* SPECS_ROGUE[] = { "Assassination", "Outlaw", "Subtlety" , "Class tree (Assassination)", "Class tree (Outlaw)", "Class tree (Subtlety)" };
    static const char* SPECS_SHAMAN[] = { "Elemental", "Enhancement", "Restoration" , "Class tree (Elemental)", "Class tree (Enhancement)", "Class tree (Restoration)" };
    static const char* SPECS_WARLOCK[] = { "Affliction", "Demonology", "Destruction" , "Class tree (Affliction)", "Class tree (Demonology)", "Class tree (Destruction)" };
    static const char* SPECS_WARRIOR[] = { "Arms", "Fury", "Protection" , "Class tree (Arms)", "Class tree (Fury)", "Class tree (Protection)" };

    const int RETURN_SPEC_COUNT(int classID);
    const char** RETURN_SPECS(int classID);
    const std::string RETURN_PRESET(std::map<std::string, std::string> presets, int classID, int specID);

    std::filesystem::path getAppPath();
    std::map<std::string, std::string> LOAD_PRESETS();

    /*
    First line (";" is line separator, ":" separates different parts of a single line, "," separates individual components of a property):
        1. creation version
        2. custom or spec preset, if spec preset then spec name all lowercase, only letters, spaces transformed to "_" (just like in simc)
            a. if preset then all the talent point definitions can be omitted if preset is stored in a library (I build this library up with a custom string but that's implementation detail)
        3. Tree type (0 for class tree, 1 for spec tree)
        4. Tree name
        5. Tree description
        6. Loadout description (a loadout is a list of skill sets, e.g. "single target", "aoe", that get defined at the end)
        7. number of talents in the tree
        8. number of loadouts

    Then N lines for N talents:
        1. Index (doesn't have to be starting from 0 and in order but loadouts define talent points spent for each talent ordered by index so it's easier to keep it that way)
        2. Talent name (does not have to be "A1".. this is the real talent name, if switch talent then we have 2 names separated by ",")
        3. Talent descriptions (1 description for each maxRank, or 2 if switch talent, separated by ",")
        4. Talent type (0 is active, 1 is passive, 2 is "switch")
        5. Row position
        6. Column position
        7. Max points
        8. Points required
        9. Pre-filled (0 for no, 1 for yes)
        10. Parent indices
        11. Child indices

    Then N lines for N loadouts:
        1. Loadout name
        2. Points spent in talent (max is maxRank or 2 for switch talent. switch talent is either 0 for not selected, 1 for first, 2 for second)
            a. The points are in order for ordered talents (ordered by index)
    */
}