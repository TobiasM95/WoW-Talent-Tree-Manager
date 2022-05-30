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
        DRUID_SPEC_IDS_RESTORATION = 3,
        DRUID_SPEC_IDS_CLASSIC = 4
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
    static const char* SPECS_DRUID[] = {"Balance", "Feral", "Guardian", "Restoration", "Classic"};
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
    const std::string RETURN_PRESET(int classID, int specID);
    const std::string RETURN_PRESET_BY_NAME(std::string presetNAME);

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
    static const std::string NEW_CUSTOM_TREE_PRESET =
        "custom:New custom Tree:"
        "Welcome to WoW Talent Tree Manager.__n__This is a solution to manage trees and their loadouts. A loadout is a collection of skillsets, __n__e.g. a skillset for Raid/M+/PvP etc. You can edit trees or work with spec presets, manage __n__various skillsets in your loadout as well as auto solve all possible skill combinations with a __n__given filter.__n____n__Edit the tree name/description here. To edit tree nodes select \"Tree Editor\" in the top right.__n__Press \"Save/Load Trees\" to load tree spec presets, manage your custom trees and import or__n__export trees (from/to Discord, etc.).__n__To start the tree editing process, go to \"Tree Editor\" -> \"Create Node\" and create your first talent.__n____n__Select the \"Talent Loadout Editor\" in the top left to manage your loadout. You can __n__edit the loadout description there and create/import/export skillsets. Since skillsets are stored__n__inside the loadout which is part of the tree, you can save your skillsets by saving the tree__n__in the tree editor.__n____n__Lastly, select the \"Talent Loadout Solver\" to generate all possible combinations of talent__n__selections for all possible amounts of spendable talent points. Afterwards, you can filter__n__the results to include/exclude specific talents and load the results into your loadout.__n____n__Hint__cl__ This text can be edited!:"
        "Your loadout is a collection of different skillset that you can edit to suit various ingame situations,__n__e.g. a raid setup, an M+ setup or different PvP skillsets.__n__All skillsets are stored in the loadout which in turn is stored in the tree. So if you save your tree__n__you'll save your loadout as well! Additionally, you can import/export skillsets directly, to share__n__with friends or your favorite discord class experts.:"
        "0:0;";

    static const std::string DEATHKNIGHT_BLOOD_PRESET =
        "deathknight_blood:Blood Deathknight:This is the preset for the Blood Deathknight.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string DEATHKNIGHT_FROST_PRESET =
        "deathknight_frost:Frost Deathknight:This is the preset for the Frost Deathknight.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string DEATHKNIGHT_UNHOLY_PRESET =
        "deathknight_unholy:Unholy Deathknight:This is the preset for the Unholy Deathknight.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string DEMONHUNTER_HAVOC_PRESET =
        "demonhunter_havoc:Havoc Demonhunter:This is the preset for the Havoc Demonhunter.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string DEMONHUNTER_VENGEANCE_PRESET =
        "demonhunter_vengeance:Vengeance Demonhunter:This is the preset for the Vengeance Demonhunter.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string DRUID_BALANCE_PRESET =
        "druid_balance:Balance Druid:This is the preset for the Balance Druid.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string DRUID_FERAL_PRESET =
        "druid_feral:Feral Druid:This is the preset for the Feral Druid.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string DRUID_GUARDIAN_PRESET =
        "druid_guardian:Guardian Druid:This is the preset for the Guardian Druid.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string DRUID_RESTORATION_PRESET =
        "druid_restoration:Restoration Druid:This is the preset for the Restoration Druid.__n__You can start editing the tree/loadout now.::31:0;"
        "0:A1_PLACEHOLDER:DA1:0:1:4:1:0::1,2,3;"
        "1:B1_PLACEHOLDER:DB1:0:2:3:1:0:0:4;"
        "2:B2_PLACEHOLDER:DB2,DB22:1:2:4:2:0:0:5;"
        "3:B3_PLACEHOLDER:DB3:1:2:5:1:0:0:6;"
        "4:C1_PLACEHOLDER:DC1:0:3:2:1:0:1:7,10;"
        "5:C2_PLACEHOLDER:DC2:0:3:4:1:0:2:;"
        "6:C3_PLACEHOLDER:DC3:0:3:6:1:0:3:8,9,12;"
        "7:D1_PLACEHOLDER:DD1,DD12:1:4:3:2:0:4:11;"
        "8:D2_PLACEHOLDER:DD2,DD22:1:4:5:2:0:6:11;"
        "9:D3_PLACEHOLDER:DD3,DD32:1:4:7:2:0:6:;"
        "10:E1_PLACEHOLDER:DE1,DE12,DE13:1:5:2:3:0:4:13;"
        "11:E2_PLACEHOLDER,SE2_PLACEHOLDER:DE2,DSE2:2:5:4:1:0:7,8:14,15;"
        "12:E4_PLACEHOLDER:DE4:1:5:6:1:0:6:16;"
        "13:F1_PLACEHOLDER:DF1:1:6:2:1:0:10:17,20;"
        "14:F2_PLACEHOLDER:DF2,DF22:1:6:3:2:0:11:17;"
        "15:F3_PLACEHOLDER:DF3,DF32:1:6:5:2:0:11:18;"
        "16:F4_PLACEHOLDER:DF4:1:6:6:1:0:12:18,19;"
        "17:G1_PLACEHOLDER,SG1_PLACEHOLDER:DG1,DSG1:2:7:3:1:0:13,14:21;"
        "18:G3_PLACEHOLDER:DG3:1:7:5:1:0:15,16:21;"
        "19:G4_PLACEHOLDER:DG4,DG42:1:7:6:2:0:16:22;"
        "20:H1_PLACEHOLDER,SH1_PLACEHOLDER:DH1,DSH1:2:8:2:1:0:13:23,24,25;"
        "21:H3_PLACEHOLDER:DH3:1:8:4:1:0:17,18:25,26;"
        "22:H4_PLACEHOLDER:DH4:0:8:6:1:0:19:26,27;"
        "23:I1_PLACEHOLDER:DI1:1:9:1:1:0:20:28;"
        "24:I2_PLACEHOLDER:DI2:1:9:2:1:0:20:;"
        "25:I3_PLACEHOLDER:DI3,DI32:1:9:3:2:0:20,21:29;"
        "26:I4_PLACEHOLDER:DI4,DI42:1:9:5:2:0:21,22:29;"
        "27:I5_PLACEHOLDER:DI5:1:9:7:1:0:22:30;"
        "28:J1_PLACEHOLDER,SJ1_PLACEHOLDER:DJ1,DSJ1:2:10:1:1:0:23:;"
        "29:J3_PLACEHOLDER,SJ1_PLACEHOLDER:DJ3,DSJ1:2:10:4:1:0:25,26:;"
        "30:J5_PLACEHOLDER,SJ1_PLACEHOLDER:DJ5,DSJ1:2:10:7:1:0:27:;";

    static const std::string DRUID_CLASSIC_PRESET =
        "druid_classic:Druid Classic:This is the standard tree preset for druid_classic.:Create your loadouts here:47:0;"
        "0:Improved Wrath:Reduces the cast time of your Wrath spell by 0.1 sec.,Reduces the cast time of your Wrath spell by 0.2 sec.,Reduces the cast time of your Wrath spell by 0.3 sec.,Reduces the cast time of your Wrath spell by 0.4 sec.,Reduces the cast time of your Wrath spell by 0.5 sec.:1:1:1:5:0::;"
        "1:Nature's Grasp:While active__cm__ any time an enemy strikes the caster they have a 35% chance to become afflicted by Entangling Roots (Rank 1). Only useable outdoors. 1 charge. Lasts 45 sec.:1:1:2:1:0::2;"
        "2:Improved Nature's Grasp:Increases the chance for your Nature's Grasp to entangle an enemy by 15%.,Increases the chance for your Nature's Grasp to entangle an enemy by 30%.,Increases the chance for your Nature's Grasp to entangle an enemy by 45%.,Increases the chance for your Nature's Grasp to entangle an enemy by 60%.:1:1:3:4:1:1:;"
        "3:Ferocity:Reduces the cost of your Maul__cm__ Swipe__cm__ Claw__cm__ and Rake abilities by 1 Rage or Energy.,Reduces the cost of your Maul__cm__ Swipe__cm__ Claw__cm__ and Rake abilities by 2 Rage or Energy.,Reduces the cost of your Maul__cm__ Swipe__cm__ Claw__cm__ and Rake abilities by 3 Rage or Energy.,Reduces the cost of your Maul__cm__ Swipe__cm__ Claw__cm__ and Rake abilities by 4 Rage or Energy.,Reduces the cost of your Maul__cm__ Swipe__cm__ Claw__cm__ and Rake abilities by 5 Rage or Energy.:1:1:7:5:0::;"
        "4:Feral Aggression:Increases the Attack Power reduction of your Demoralizing Roar by 8% and the damage caused by your Ferocious Bite by 3%.,Increases the Attack Power reduction of your Demoralizing Roar by 16% and the damage caused by your Ferocious Bite by 6%.,Increases the Attack Power reduction of your Demoralizing Roar by 24% and the damage caused by your Ferocious Bite by 9%.,Increases the Attack Power reduction of your Demoralizing Roar by 32% and the damage caused by your Ferocious Bite by 12%.,Increases the Attack Power reduction of your Demoralizing Roar by 40% and the damage caused by your Ferocious Bite by 15%.:1:1:8:5:0::;"
        "5:Improved Mark of the Wild:Increases the effects of your Mark of the Wild and Gift of the Wild spells by 7%.,Increases the effects of your Mark of the Wild and Gift of the Wild spells by 14%.,Increases the effects of your Mark of the Wild and Gift of the Wild spells by 21%.,Increases the effects of your Mark of the Wild and Gift of the Wild spells by 28%.,Increases the effects of your Mark of the Wild and Gift of the Wild spells by 35%.:1:1:12:5:0::;"
        "6:Furor:Gives you 20% chance to gain 10 Rage when you shapeshift into Bear and Dire Bear Form or 40 Energy when you shapeshift into Cat Form.,Gives you 40% chance to gain 10 Rage when you shapeshift into Bear and Dire Bear Form or 40 Energy when you shapeshift into Cat Form.,Gives you 60% chance to gain 10 Rage when you shapeshift into Bear and Dire Bear Form or 40 Energy when you shapeshift into Cat Form.,Gives you 80% chance to gain 10 Rage when you shapeshift into Bear and Dire Bear Form or 40 Energy when you shapeshift into Cat Form.,Gives you 100% chance to gain 10 Rage when you shapeshift into Bear and Dire Bear Form or 40 Energy when you shapeshift into Cat Form.:1:1:13:5:0::;"
        "7:Improved Entangling Roots:Gives you a 40% chance to avoid interruption caused by damage while casting Entangling Roots.,Gives you a 70% chance to avoid interruption caused by damage while casting Entangling Roots.,Gives you a 100% chance to avoid interruption caused by damage while casting Entangling Roots.:1:2:1:3:5::;"
        "8:Improved Moonfire:Increases the damage and critical strike chance of your Moonfire spell by 2%.,Increases the damage and critical strike chance of your Moonfire spell by 4%.,Increases the damage and critical strike chance of your Moonfire spell by 6%.,Increases the damage and critical strike chance of your Moonfire spell by 8%.,Increases the damage and critical strike chance of your Moonfire spell by 10%.:1:2:2:5:5::26;"
        "9:Natural Weapons:Increases the damage you deal with physical attacks in all forms by 2%.,Increases the damage you deal with physical attacks in all forms by 4%.,Increases the damage you deal with physical attacks in all forms by 6%.,Increases the damage you deal with physical attacks in all forms by 8%.,Increases the damage you deal with physical attacks in all forms by 10%.:1:2:3:5:5::18;"
        "10:Natural Shapeshifter:Reduces the mana cost of all shapeshifting by 10%.,Reduces the mana cost of all shapeshifting by 20%.,Reduces the mana cost of all shapeshifting by 30%.:1:2:4:3:5::;"
        "11:Feral Instinct:Increases threat caused in Bear and Dire Bear Form by 3% and reduces the chance enemies have to detect you while Prowling.,Increases threat caused in Bear and Dire Bear Form by 6% and reduces the chance enemies have to detect you while Prowling.,Increases threat caused in Bear and Dire Bear Form by 9% and reduces the chance enemies have to detect you while Prowling.,Increases threat caused in Bear and Dire Bear Form by 12% and reduces the chance enemies have to detect you while Prowling.,Increases threat caused in Bear and Dire Bear Form by 15% and reduces the chance enemies have to detect you while Prowling.:1:2:6:5:5::;"
        "12:Brutal Impact:Increases the stun duration of your Bash and Pounce abilities by 0.5 sec.,Increases the stun duration of your Bash and Pounce abilities by 1 sec.:1:2:7:2:5::;"
        "13:Thick Hide:Increases your Armor contribution from items by 2%.,Increases your Armor contribution from items by 4%.,Increases your Armor contribution from items by 6%.,Increases your Armor contribution from items by 8%.,Increases your Armor contribution from items by 10%.:1:2:8:5:5::;"
        "14:Improved Healing Touch:Reduces the cast time of your Healing Touch spell by 0.1 sec.,Reduces the cast time of your Healing Touch spell by 0.2 sec.,Reduces the cast time of your Healing Touch spell by 0.3 sec.,Reduces the cast time of your Healing Touch spell by 0.4 sec.,Reduces the cast time of your Healing Touch spell by 0.5 sec.:1:2:11:5:5::38;"
        "15:Nature's Focus:Gives you a 14% chance to avoid interruption caused by damage while casting the Healing Touch__cm__ Regrowth and Tranquility spells.,Gives you a 28% chance to avoid interruption caused by damage while casting the Healing Touch__cm__ Regrowth and Tranquility spells.,Gives you a 42% chance to avoid interruption caused by damage while casting the Healing Touch__cm__ Regrowth and Tranquility spells.,Gives you a 56% chance to avoid interruption caused by damage while casting the Healing Touch__cm__ Regrowth and Tranquility spells.,Gives you a 70% chance to avoid interruption caused by damage while casting the Healing Touch__cm__ Regrowth and Tranquility spells.:1:2:12:5:5::;"
        "16:Improved Enrage:The Enrage ability now instantly generates 5 Rage.,The Enrage ability now instantly generates 10 Rage.:1:2:13:2:5::;"
        "17:Improved Thorns:Increases damage caused by your Thorns spell by 25%.,Increases damage caused by your Thorns spell by 50%.,Increases damage caused by your Thorns spell by 75%.:1:3:1:3:10::;"
        "18:Omen of Clarity:Imbues the Druid with natural energy. Each of the Druid's melee attacks has a chance of causing the caster to enter a Clearcasting state. The Clearcasting state reduces the Mana__cm__ Rage or Energy cost of your next damage or healing spell or offensive ability by 100%. Lasts 10 min.:0:3:3:1:10:9:;"
        "19:Nature's Reach:Increases the range of your Wrath__cm__ Entangling Roots__cm__ Faerie Fire__cm__ Moonfire__cm__ Starfire__cm__ and Hurricane spells by 10%.,Increases the range of your Wrath__cm__ Entangling Roots__cm__ Faerie Fire__cm__ Moonfire__cm__ Starfire__cm__ and Hurricane spells by 20%.:1:3:4:2:10::;"
        "20:Feline Swiftness:Increases your movement speed by 15% while outdoors in Cat Form and increases your chance to dodge while in Cat Form by 2%.,Increases your movement speed by 30% while outdoors in Cat Form and increases your chance to dodge while in Cat Form by 4%.:1:3:6:2:10::;"
        "21:Feral Charge:Causes you to charge an enemy__cm__ immobilizing and interrupting any spell being cast for 4 sec.:0:3:7:1:10::;"
        "22:Sharpened Claws:Increases your critical strike chance while in Bear__cm__ Dire Bear or Cat Form by 2%.,Increases your critical strike chance while in Bear__cm__ Dire Bear or Cat Form by 4%.,Increases your critical strike chance while in Bear__cm__ Dire Bear or Cat Form by 6%.:1:3:8:3:10::30,31;"
        "23:Reflection:Allows 5% of your Mana regeneration to continue while casting.,Allows 10% of your Mana regeneration to continue while casting.,Allows 15% of your Mana regeneration to continue while casting.:1:3:12:3:10::;"
        "24:Insect Swarm:The enemy target is swarmed by insects__cm__ decreasing their chance to hit by 2% and causing 66 Nature damage over 12 sec.:1:3:13:1:10::39;"
        "25:Subtlety:Reduces the threat generated by your Healing spells by 4%.,Reduces the threat generated by your Healing spells by 8%.,Reduces the threat generated by your Healing spells by 12%.,Reduces the threat generated by your Healing spells by 16%.,Reduces the threat generated by your Healing spells by 20%.:1:3:14:5:10::;"
        "26:Vengeance:Increases the critical strike damage bonus of your Starfire__cm__ Moonfire__cm__ and Wrath spells by 20%.,Increases the critical strike damage bonus of your Starfire__cm__ Moonfire__cm__ and Wrath spells by 40%.,Increases the critical strike damage bonus of your Starfire__cm__ Moonfire__cm__ and Wrath spells by 60%.,Increases the critical strike damage bonus of your Starfire__cm__ Moonfire__cm__ and Wrath spells by 80%.,Increases the critical strike damage bonus of your Starfire__cm__ Moonfire__cm__ and Wrath spells by 100%.:1:4:2:5:15:8:;"
        "27:Improved Starfire:Reduces the cast time of Starfire by 0.1 sec and has a 3% chance to stun the target for 3 sec.,Reduces the cast time of Starfire by 0.2 sec and has a 6% chance to stun the target for 3 sec.,Reduces the cast time of Starfire by 0.3 sec and has a 9% chance to stun the target for 3 sec.,Reduces the cast time of Starfire by 0.4 sec and has a 12% chance to stun the target for 3 sec.,Reduces the cast time of Starfire by 0.5 sec and has a 15% chance to stun the target for 3 sec.:1:4:3:5:15::;"
        "28:Improved Shred:Reduces the Energy cost of your Shred ability by 6.,Reduces the Energy cost of your Shred ability by 12.:1:4:6:2:15::;"
        "29:Predatory Strikes:Increases your melee attack power in Cat__cm__ Bear and Dire Bear Forms by 50% of your level.,Increases your melee attack power in Cat__cm__ Bear and Dire Bear Forms by 100% of your level.,Increases your melee attack power in Cat__cm__ Bear and Dire Bear Forms by 150% of your level.:1:4:7:3:15::42;"
        "30:Blood Frenzy:Your critical strikes from Cat Form abilities that add combo points have a 50% chance to add an additional combo point.,Your critical strikes from Cat Form abilities that add combo points have a 100% chance to add an additional combo point.:1:4:8:2:15:22:;"
        "31:Primal Fury:Gives you a 50% chance to gain an additional 5 Rage anytime you get a critical strike while in Bear and Dire Bear Form.,Gives you a 100% chance to gain an additional 5 Rage anytime you get a critical strike while in Bear and Dire Bear Form.:1:4:9:2:15:22:;"
        "32:Tranquil Spirit:Reduces the mana cost of your Healing Touch and Tranquility spells by 2%.,Reduces the mana cost of your Healing Touch and Tranquility spells by 4%.,Reduces the mana cost of your Healing Touch and Tranquility spells by 6%.,Reduces the mana cost of your Healing Touch and Tranquility spells by 8%.,Reduces the mana cost of your Healing Touch and Tranquility spells by 10%.:1:4:12:5:15::46;"
        "33:Improved Rejuvenation:Increases the effect of your Rejuvenation spell by 5%.,Increases the effect of your Rejuvenation spell by 10%.,Increases the effect of your Rejuvenation spell by 15%.:1:4:14:3:15::;"
        "34:Nature's Grace:All spell criticals grace you with a blessing of nature__cm__ reducing the casting time of your next spell by 0.5 sec.:1:5:2:1:20::41;"
        "35:Moonglow:Reduces the Mana cost of your Moonfire__cm__ Starfire__cm__ Wrath__cm__ Healing Touch__cm__ Regrowth and Rejuvenation spells by 3%.,Reduces the Mana cost of your Moonfire__cm__ Starfire__cm__ Wrath__cm__ Healing Touch__cm__ Regrowth and Rejuvenation spells by 6%.,Reduces the Mana cost of your Moonfire__cm__ Starfire__cm__ Wrath__cm__ Healing Touch__cm__ Regrowth and Rejuvenation spells by 9%.:1:5:3:3:20::;"
        "36:Savage Fury:Increases the damage caused by your Claw__cm__ Rake__cm__ Maul and Swipe abilities by 10%.,Increases the damage caused by your Claw__cm__ Rake__cm__ Maul and Swipe abilities by 20%.:1:5:6:2:20::;"
        "37:Faerie Fire (Feral):Decrease the armor of the target by 175 for 40 sec. While affected__cm__ the target cannot stealth or turn invisible.:0:5:8:1:20::;"
        "38:Nature's Swiftness:When activated__cm__ your next Nature spell becomes an instant cast spell.:1:5:11:1:20:14:;"
        "39:Gift of Nature:Increases the effect of all healing spells by 2%.,Increases the effect of all healing spells by 4%.,Increases the effect of all healing spells by 6%.,Increases the effect of all healing spells by 8%.,Increases the effect of all healing spells by 10%.:1:5:13:5:20:24:;"
        "40:Improved Tranquility:Reduces threat caused by Tranquility by 50%.,Reduces threat caused by Tranquility by 100%.:1:5:14:2:20::;"
        "41:Moonfury:Increases the damage done by your Starfire__cm__ Moonfire and Wrath spells by 2%.,Increases the damage done by your Starfire__cm__ Moonfire and Wrath spells by 4%.,Increases the damage done by your Starfire__cm__ Moonfire and Wrath spells by 6%.,Increases the damage done by your Starfire__cm__ Moonfire and Wrath spells by 8%.,Increases the damage done by your Starfire__cm__ Moonfire and Wrath spells by 10%.:1:6:2:5:25:34:;"
        "42:Heart of the Wild:Increases your Intellect by 4%. In addition__cm__ while in Bear or Dire Bear Form your Stamina is increased by 4% and while in Cat Form your Strength is increased by 4%.,Increases your Intellect by 8%. In addition__cm__ while in Bear or Dire Bear Form your Stamina is increased by 8% and while in Cat Form your Strength is increased by 8%.,Increases your Intellect by 12%. In addition__cm__ while in Bear or Dire Bear Form your Stamina is increased by 12% and while in Cat Form your Strength is increased by 12%.,Increases your Intellect by 16%. In addition__cm__ while in Bear or Dire Bear Form your Stamina is increased by 16% and while in Cat Form your Strength is increased by 16%.,Increases your Intellect by 20%. In addition__cm__ while in Bear or Dire Bear Form your Stamina is increased by 20% and while in Cat Form your Strength is increased by 20%.:1:6:7:5:25:29:;"
        "43:Improved Regrowth:Increases the critical effect chance of your Regrowth spell by 10%.,Increases the critical effect chance of your Regrowth spell by 20%.,Increases the critical effect chance of your Regrowth spell by 30%.,Increases the critical effect chance of your Regrowth spell by 40%.,Increases the critical effect chance of your Regrowth spell by 50%.:1:6:13:5:25::;"
        "44:Moonkin Form:Transforms the Druid into Moonkin Form. While in this form the armor contribution from items is increased by 360% and all party members within 30 yards have their spell critical chance increased by 3%. The Moonkin can only cast Balance spells while shapeshifted.__n____n__The act of shapeshifting frees the caster of Polymorph and Movement Impairing effects.:0:7:2:1:30::;"
        "45:Leader of the Pack:While in Cat__cm__ Bear or Dire Bear Form__cm__ the Leader of the Pack increases ranged and melee critical chance of all party members within 45 yards by 3%.:1:7:7:1:30::;"
        "46:Swiftmend:Consumes a Rejuvenation or Regrowth effect on a friendly target to instantly heal them an amount equal to 12 sec. of Rejuvenation or 18 sec. of Regrowth.:0:7:12:1:30:32:;";

    static const std::string EVOKER_DEVASTATION_PRESET =
        "evoker_devastation:Devastation Evoker:This is the preset for the Devastation Evoker.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string EVOKER_PRESERVATION_PRESET =
        "evoker_preservation:Preservation Evoker:This is the preset for the Preservation Evoker.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string HUNTER_BEASTMASTERY_PRESET =
        "hunter_beastmastery:Beastmastery Hunter:This is the preset for the Beastmastery Hunter.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string HUNTER_MARKSMANSHIP_PRESET =
        "hunter_marksmanship:Marksmanship Hunter:This is the preset for the Marksmanship Hunter.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string HUNTER_SURVIVAL_PRESET =
        "hunter_survival:Survival Hunter:This is the preset for the Survival Hunter.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string MAGE_ARCANE_PRESET =
        "mage_arcane:Arcane Mage:This is the preset for the Arcane Mage.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string MAGE_FIRE_PRESET =
        "mage_fire:Fire Mage:This is the preset for the Fire Mage.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string MAGE_FROST_PRESET =
        "mage_frost:Frost Mage:This is the preset for the Frost Mage.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string MONK_BREWMASTER_PRESET =
        "monk_brewmaster:Brewmaster Monk:This is the preset for the Brewmaster Monk.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string MONK_MISTWEAVER_PRESET =
        "monk_mistweaver:Mistweaver Monk:This is the preset for the Mistweaver Monk.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string MONK_WINDWALKER_PRESET =
        "monk_windwalker:Windwalker Monk:This is the preset for the Windwalker Monk.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string PALADIN_HOLY_PRESET =
        "paladin_holy:Holy Paladin:This is the preset for the Holy Paladin.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string PALADIN_PROTECTION_PRESET =
        "paladin_protection:Protection Paladin:This is the preset for the Protection Paladin.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string PALADIN_RETRIBUTION_PRESET =
        "paladin_retribution:Retribution Paladin:This is the preset for the Retribution Paladin.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string PRIEST_DISCIPLINE_PRESET =
        "priest_discipline:Discipline Priest:This is the preset for the Discipline Priest.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string PRIEST_HOLY_PRESET =
        "priest_holy:Holy Priest:This is the preset for the Holy Priest.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string PRIEST_SHADOW_PRESET =
        "priest_shadow:Shadow Priest:This is the preset for the Shadow Priest.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string ROGUE_ASSASSINATION_PRESET =
        "rogue_assassination:Assassination Rogue:This is the preset for the Assassination Rogue.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string ROGUE_OUTLAW_PRESET =
        "rogue_outlaw:Outlaw Rogue:This is the preset for the Outlaw Rogue.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string ROGUE_SUBTLETY_PRESET =
        "rogue_subtlety:Subtlety Rogue:This is the preset for the Subtlety Rogue.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string SHAMAN_ELEMENTAL_PRESET =
        "shaman_elemental:Elemental Shaman:This is the preset for the Elemental Shaman.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string SHAMAN_ENHANCEMENT_PRESET =
        "shaman_enhancement:Enhancement Shaman:This is the preset for the Enhancement Shaman.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string SHAMAN_RESTORATION_PRESET =
        "shaman_restoration:Restoration Shaman:This is the preset for the Restoration Shaman.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string WARLOCK_AFFLICTION_PRESET =
        "warlock_affliction:Affliction Warlock:This is the preset for the Affliction Warlock.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string WARLOCK_DEMONOLOGY_PRESET =
        "warlock_demonology:Demonology Warlock:This is the preset for the Demonology Warlock.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string WARLOCK_DESTRUCTION_PRESET =
        "warlock_destruction:Destruction Warlock:This is the preset for the Destruction Warlock.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string WARRIOR_ARMS_PRESET =
        "warrior_arms:Arms Warrior:This is the preset for the Arms Warrior.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string WARRIOR_FURY_PRESET =
        "warrior_fury:Fury Warrior:This is the preset for the Fury Warrior.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";

    static const std::string WARRIOR_PROTECTION_PRESET =
        "warrior_protection:Protection Warrior:This is the preset for the Protection Warrior.__n__You can start editing the tree/loadout now.::1:0;"
        "0:PLACEHOLDER:Placeholder talent__cl__Please wait for the release of the talent trees and check the github page for a new version.:0:1:4:1:0::;";
}