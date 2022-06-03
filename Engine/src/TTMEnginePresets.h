#pragma once

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
        DEATHKNIGHT_SPEC_IDS_UNHOLY = 2
    };

    enum DEMONHUNTER_SPEC_IDS {
        DEMONHUNTER_SPEC_IDS_HAVOC = 0,
        DEMONHUNTER_SPEC_IDS_VENGEANCE = 1
    };

    enum DRUID_SPEC_IDS {
        DRUID_SPEC_IDS_BALANCE = 0,
        DRUID_SPEC_IDS_FERAL = 1,
        DRUID_SPEC_IDS_GUARDIAN = 2,
        DRUID_SPEC_IDS_RESTORATION = 3
    };

    enum EVOKER_SPEC_IDS {
        EVOKER_SPEC_IDS_DEVASTATION = 0,
        EVOKER_SPEC_IDS_PRESERVATION = 1
    };

    enum HUNTER_SPEC_IDS {
        HUNTER_SPEC_IDS_BEASTMASTERY = 0,
        HUNTER_SPEC_IDS_MARKSMANSHIP = 1,
        HUNTER_SPEC_IDS_SURVIVAL = 2
    };

    enum MAGE_SPEC_IDS {
        MAGE_SPEC_IDS_ARCANE = 0,
        MAGE_SPEC_IDS_FIRE = 1,
        MAGE_SPEC_IDS_FROST = 2
    };

    enum MONK_SPEC_IDS {
        MONK_SPEC_IDS_BREWMASTER = 0,
        MONK_SPEC_IDS_MISTWEAVER = 1,
        MONK_SPEC_IDS_WINDWALKER = 2
    };

    enum PALADIN_SPEC_IDS {
        PALADIN_SPEC_IDS_HOLY = 0,
        PALADIN_SPEC_IDS_PROTECTION = 1,
        PALADIN_SPEC_IDS_RETRIBUTION = 2
    };

    enum PRIEST_SPEC_IDS {
        PRIEST_SPEC_IDS_DISCIPLINE = 0,
        PRIEST_SPEC_IDS_HOLY = 1,
        PRIEST_SPEC_IDS_SHADOW = 2
    };

    enum ROGUE_SPEC_IDS {
        ROGUE_SPEC_IDS_ASSASSINATION = 0,
        ROGUE_SPEC_IDS_OUTLAW = 1,
        ROGUE_SPEC_IDS_SUBTLETY = 2
    };

    enum SHAMAN_SPEC_IDS {
        SHAMAN_SPEC_IDS_ELEMENTAL = 0,
        SHAMAN_SPEC_IDS_ENHANCEMENT = 1,
        SHAMAN_SPEC_IDS_RESTORATION = 2
    };

    enum WARLOCK_SPEC_IDS {
        WARLOCK_SPEC_IDS_AFFLICTION = 0,
        WARLOCK_SPEC_IDS_DEMONOLOGY = 1,
        WARLOCK_SPEC_IDS_DESTRUCTION = 2
    };

    enum WARRIOR_SPEC_IDS {
        WARRIOR_SPEC_IDS_ARMS = 0,
        WARRIOR_SPEC_IDS_FURY = 1,
        WARRIOR_SPEC_IDS_PROTECTION = 2
    };

    static const char* CLASSES[] = { "Death Knight", "Demon Hunter", "Druid", "Evoker", "Hunter", "Mage", "Monk", "Paladin", "Priest", "Rogue", "Shaman", "Warlock", "Warrior"};
    static const char* SPECS_DEATHKNIGHT[] = { "Blood", "Frost", "Unholy" };
    static const char* SPECS_DEMONHUNTER[] = { "Havoc", "Vengeance" };
    static const char* SPECS_DRUID[] = {"Balance", "Feral", "Guardian", "Restoration"};
    static const char* SPECS_EVOKER[] = { "Devastation" , "Preservation"};
    static const char* SPECS_HUNTER[] = { "Beast Mastery", "Marksmanship", "Survival" };
    static const char* SPECS_MAGE[] = { "Arcane", "Fire", "Frost" };
    static const char* SPECS_MONK[] = { "Brewmaster", "Mistweaver", "Windwalker" };
    static const char* SPECS_PALADIN[] = { "Holy", "Protection", "Retribution" };
    static const char* SPECS_PRIEST[] = { "Discipline", "Holy", "Shadow" };
    static const char* SPECS_ROGUE[] = { "Assassination", "Outlaw", "Subtlety" };
    static const char* SPECS_SHAMAN[] = { "Elemental", "Enhancement", "Restoration" };
    static const char* SPECS_WARLOCK[] = { "Affliction", "Demonology", "Destruction" };
    static const char* SPECS_WARRIOR[] = { "Arms", "Fury", "Protection" };

    const int RETURN_SPEC_COUNT(int classID);
    const char** RETURN_SPECS(int classID);
    const std::string RETURN_PRESET(std::map<std::string, std::string> presets, int classID, int specID);

    std::map<std::string, std::string> LOAD_PRESETS();

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
}