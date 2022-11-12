/*
	WoW Talent Tree Manager is an application for creating/editing/sharing talent trees and setups.
	Copyright(C) 2022 Tobias Mielich

	This program is free software : you can redistribute it and /or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see < https://www.gnu.org/licenses/>.

	Contact via https://github.com/TobiasM95/WoW-Talent-Tree-Manager/discussions or BuffMePls#2973 on Discord
*/

#include "TTMEnginePresets.h"

#include <windows.h>
#include <shlobj.h>

#include <fstream>
#include <stdexcept>

namespace Presets {

	const int RETURN_SPEC_COUNT(int classID) {
		switch (classID) {
		case CLASS_IDS_DEATHKNIGHT: return 6;
		case CLASS_IDS_DEMONHUNTER: return 4;
		case CLASS_IDS_DRUID: return 8;
		case CLASS_IDS_EVOKER: return 4;
		case CLASS_IDS_HUNTER: return 6;
		case CLASS_IDS_MAGE: return 6;
		case CLASS_IDS_MONK: return 6;
		case CLASS_IDS_PALADIN: return 6;
		case CLASS_IDS_PRIEST: return 6;
		case CLASS_IDS_ROGUE: return 6;
		case CLASS_IDS_SHAMAN: return 6;
		case CLASS_IDS_WARLOCK: return 6;
		case CLASS_IDS_WARRIOR: return 6;
		default: throw std::logic_error("Class ID does not exist!");
		}
	}

	const char** RETURN_SPECS(int classID) {
		switch (classID) {
		case CLASS_IDS_DEATHKNIGHT: return SPECS_DEATHKNIGHT;
		case CLASS_IDS_DEMONHUNTER: return SPECS_DEMONHUNTER;
		case CLASS_IDS_DRUID: return SPECS_DRUID;
		case CLASS_IDS_EVOKER: return SPECS_EVOKER;
		case CLASS_IDS_HUNTER: return SPECS_HUNTER;
		case CLASS_IDS_MAGE: return SPECS_MAGE;
		case CLASS_IDS_MONK: return SPECS_MONK;
		case CLASS_IDS_PALADIN: return SPECS_PALADIN;
		case CLASS_IDS_PRIEST: return SPECS_PRIEST;
		case CLASS_IDS_ROGUE: return SPECS_ROGUE;
		case CLASS_IDS_SHAMAN: return SPECS_SHAMAN;
		case CLASS_IDS_WARLOCK: return SPECS_WARLOCK;
		case CLASS_IDS_WARRIOR: return SPECS_WARRIOR;
		default: throw std::logic_error("Class ID does not exist!");
		}
	}

	const std::string RETURN_PRESET(std::map<std::string, std::string> presets, int classID, int specID) {
		switch (classID) {
		case CLASS_IDS_DEATHKNIGHT: {
			switch (specID) {
			case DEATHKNIGHT_SPEC_IDS_BLOOD: return presets["deathknight_blood"];
			case DEATHKNIGHT_SPEC_IDS_FROST: return presets["deathknight_frost"];
			case DEATHKNIGHT_SPEC_IDS_UNHOLY: return presets["deathknight_unholy"];
			case DEATHKNIGHT_SPEC_IDS_CLASS_BLOOD: return presets["deathknight_class_blood"];
			case DEATHKNIGHT_SPEC_IDS_CLASS_FROST: return presets["deathknight_class_frost"];
			case DEATHKNIGHT_SPEC_IDS_CLASS_UNHOLY: return presets["deathknight_class_unholy"];
			}
		}
		case CLASS_IDS_DEMONHUNTER: {
			switch (specID) {
			case DEMONHUNTER_SPEC_IDS_HAVOC: return presets["demonhunter_havoc"];
			case DEMONHUNTER_SPEC_IDS_VENGEANCE: return presets["demonhunter_vengeance"];
			case DEMONHUNTER_SPEC_IDS_CLASS_HAVOC: return presets["demonhunter_class_havoc"];
			case DEMONHUNTER_SPEC_IDS_CLASS_VENGEANCE: return presets["demonhunter_class_vengeance"];
			}
		}
		case CLASS_IDS_DRUID: {
			switch (specID) {
			case DRUID_SPEC_IDS_BALANCE: return presets["druid_balance"];
			case DRUID_SPEC_IDS_FERAL: return presets["druid_feral"];
			case DRUID_SPEC_IDS_GUARDIAN: return presets["druid_guardian"];
			case DRUID_SPEC_IDS_RESTORATION: return presets["druid_restoration"];
			case DRUID_SPEC_IDS_CLASS_BALANCE: return presets["druid_class_balance"];
			case DRUID_SPEC_IDS_CLASS_FERAL: return presets["druid_class_feral"];
			case DRUID_SPEC_IDS_CLASS_GUARDIAN: return presets["druid_class_guardian"];
			case DRUID_SPEC_IDS_CLASS_RESTORATION: return presets["druid_class_restoration"];
			}
		}
		case CLASS_IDS_EVOKER: {
			switch (specID) {
			case EVOKER_SPEC_IDS_DEVASTATION: return presets["evoker_devastation"];
			case EVOKER_SPEC_IDS_PRESERVATION: return presets["evoker_preservation"];
			case EVOKER_SPEC_IDS_CLASS_DEVASTATION: return presets["evoker_class_devastation"];
			case EVOKER_SPEC_IDS_CLASS_PRESERVATION: return presets["evoker_class_preservation"];
			}
		}
		case CLASS_IDS_HUNTER: {
			switch (specID) {
			case HUNTER_SPEC_IDS_BEASTMASTERY: return presets["hunter_beastmastery"];
			case HUNTER_SPEC_IDS_MARKSMANSHIP: return presets["hunter_marksmanship"];
			case HUNTER_SPEC_IDS_SURVIVAL: return presets["hunter_survival"];
			case HUNTER_SPEC_IDS_CLASS_BEASTMASTERY: return presets["hunter_class_beastmastery"];
			case HUNTER_SPEC_IDS_CLASS_MARKSMANSHIP: return presets["hunter_class_marksmanship"];
			case HUNTER_SPEC_IDS_CLASS_SURVIVAL: return presets["hunter_class_survival"];
			}
		}
		case CLASS_IDS_MAGE: {
			switch (specID) {
			case MAGE_SPEC_IDS_ARCANE: return presets["mage_arcane"];
			case MAGE_SPEC_IDS_FIRE: return presets["mage_fire"];
			case MAGE_SPEC_IDS_FROST: return presets["mage_frost"];
			case MAGE_SPEC_IDS_CLASS_ARCANCE: return presets["mage_class_arcane"];
			case MAGE_SPEC_IDS_CLASS_FIRE: return presets["mage_class_fire"];
			case MAGE_SPEC_IDS_CLASS_FROST: return presets["mage_class_frost"];
			}
		}
		case CLASS_IDS_MONK: {
			switch (specID) {
			case MONK_SPEC_IDS_BREWMASTER: return presets["monk_brewmaster"];
			case MONK_SPEC_IDS_MISTWEAVER: return presets["monk_mistweaver"];
			case MONK_SPEC_IDS_WINDWALKER: return presets["monk_windwalker"];
			case MONK_SPEC_IDS_CLASS_BREWMASTER: return presets["monk_class_brewmaster"];
			case MONK_SPEC_IDS_CLASS_MISTWEAVER: return presets["monk_class_mistweaver"];
			case MONK_SPEC_IDS_CLASS_WINDWALKER: return presets["monk_class_windwalker"];
			}
		}
		case CLASS_IDS_PALADIN: {
			switch (specID) {
			case PALADIN_SPEC_IDS_HOLY: return presets["paladin_holy"];
			case PALADIN_SPEC_IDS_PROTECTION: return presets["paladin_protection"];
			case PALADIN_SPEC_IDS_RETRIBUTION: return presets["paladin_retribution"];
			case PALADIN_SPEC_IDS_CLASS_HOLY: return presets["paladin_class_holy"];
			case PALADIN_SPEC_IDS_CLASS_PROTECTION: return presets["paladin_class_protection"];
			case PALADIN_SPEC_IDS_CLASS_RETRIBUTION: return presets["paladin_class_retribution"];
			}
		}
		case CLASS_IDS_PRIEST: {
			switch (specID) {
			case PRIEST_SPEC_IDS_DISCIPLINE: return presets["priest_discipline"];
			case PRIEST_SPEC_IDS_HOLY: return presets["priest_holy"];
			case PRIEST_SPEC_IDS_SHADOW: return presets["priest_shadow"];
			case PRIEST_SPEC_IDS_CLASS_DISCIPLINE: return presets["priest_class_discipline"];
			case PRIEST_SPEC_IDS_CLASS_HOLY: return presets["priest_class_holy"];
			case PRIEST_SPEC_IDS_CLASS_SHADOW: return presets["priest_class_shadow"];
			}
		}
		case CLASS_IDS_ROGUE: {
			switch (specID) {
			case ROGUE_SPEC_IDS_ASSASSINATION: return presets["rogue_assassination"];
			case ROGUE_SPEC_IDS_OUTLAW: return presets["rogue_outlaw"];
			case ROGUE_SPEC_IDS_SUBTLETY: return presets["rogue_subtlety"];
			case ROGUE_SPEC_IDS_CLASS_ASSASSINATION: return presets["rogue_class_assassination"];
			case ROGUE_SPEC_IDS_CLASS_OUTLAW: return presets["rogue_class_outlaw"];
			case ROGUE_SPEC_IDS_CLASS_SUBTLETY: return presets["rogue_class_subtlety"];
			}
		}
		case CLASS_IDS_SHAMAN: {
			switch (specID) {
			case SHAMAN_SPEC_IDS_ELEMENTAL: return presets["shaman_elemental"];
			case SHAMAN_SPEC_IDS_ENHANCEMENT: return presets["shaman_enhancement"];
			case SHAMAN_SPEC_IDS_RESTORATION: return presets["shaman_restoration"];
			case SHAMAN_SPEC_IDS_CLASS_ELEMENTAL: return presets["shaman_class_elemental"];
			case SHAMAN_SPEC_IDS_CLASS_ENHANCEMENT: return presets["shaman_class_enhancement"];
			case SHAMAN_SPEC_IDS_CLASS_RESTORATION: return presets["shaman_class_restoration"];
			}
		}
		case CLASS_IDS_WARLOCK: {
			switch (specID) {
			case WARLOCK_SPEC_IDS_AFFLICTION: return presets["warlock_affliction"];
			case WARLOCK_SPEC_IDS_DEMONOLOGY: return presets["warlock_demonology"];
			case WARLOCK_SPEC_IDS_DESTRUCTION: return presets["warlock_destruction"];
			case WARLOCK_SPEC_IDS_CLASS_AFFLICTION: return presets["warlock_class_affliction"];
			case WARLOCK_SPEC_IDS_CLASS_DEMONOLOGY: return presets["warlock_class_demonology"];
			case WARLOCK_SPEC_IDS_CLASS_DESTRUCTION: return presets["warlock_class_destruction"];
			}
		}
		case CLASS_IDS_WARRIOR: {
			switch (specID) {
			case WARRIOR_SPEC_IDS_ARMS: return presets["warrior_arms"];
			case WARRIOR_SPEC_IDS_FURY: return presets["warrior_fury"];
			case WARRIOR_SPEC_IDS_PROTECTION: return presets["warrior_protection"];
			case WARRIOR_SPEC_IDS_CLASS_ARMS: return presets["warrior_class_arms"];
			case WARRIOR_SPEC_IDS_CLASS_FURY: return presets["warrior_class_fury"];
			case WARRIOR_SPEC_IDS_CLASS_PROTECTION: return presets["warrior_class_protection"];
			}
		}
		default: throw std::logic_error("Combination of class ID and spec ID does not exist!");
		}
	}

	std::pair<int, int> RETURN_IDS_FROM_PRESET_NAME(const std::string& presetName) {
		if (presetName == "deathknight_blood") return { CLASS_IDS::CLASS_IDS_DEATHKNIGHT, DEATHKNIGHT_SPEC_IDS::DEATHKNIGHT_SPEC_IDS_BLOOD };
		if (presetName == "deathknight_class_blood") return { CLASS_IDS::CLASS_IDS_DEATHKNIGHT, DEATHKNIGHT_SPEC_IDS::DEATHKNIGHT_SPEC_IDS_CLASS_BLOOD };
		if (presetName == "deathknight_frost") return { CLASS_IDS::CLASS_IDS_DEATHKNIGHT, DEATHKNIGHT_SPEC_IDS::DEATHKNIGHT_SPEC_IDS_FROST };
		if (presetName == "deathknight_class_frost") return { CLASS_IDS::CLASS_IDS_DEATHKNIGHT, DEATHKNIGHT_SPEC_IDS::DEATHKNIGHT_SPEC_IDS_CLASS_FROST };
		if (presetName == "deathknight_unholy") return { CLASS_IDS::CLASS_IDS_DEATHKNIGHT, DEATHKNIGHT_SPEC_IDS::DEATHKNIGHT_SPEC_IDS_UNHOLY };
		if (presetName == "deathknight_class_unholy") return { CLASS_IDS::CLASS_IDS_DEATHKNIGHT, DEATHKNIGHT_SPEC_IDS::DEATHKNIGHT_SPEC_IDS_CLASS_UNHOLY };

		if (presetName == "demonhunter_havoc") return { CLASS_IDS::CLASS_IDS_DEMONHUNTER, DEMONHUNTER_SPEC_IDS::DEMONHUNTER_SPEC_IDS_HAVOC };
		if (presetName == "demonhunter_class_havoc") return { CLASS_IDS::CLASS_IDS_DEMONHUNTER, DEMONHUNTER_SPEC_IDS::DEMONHUNTER_SPEC_IDS_CLASS_HAVOC };
		if (presetName == "demonhunter_vengeance") return { CLASS_IDS::CLASS_IDS_DEMONHUNTER, DEMONHUNTER_SPEC_IDS::DEMONHUNTER_SPEC_IDS_VENGEANCE };
		if (presetName == "demonhunter_class_vengeance") return { CLASS_IDS::CLASS_IDS_DEMONHUNTER, DEMONHUNTER_SPEC_IDS::DEMONHUNTER_SPEC_IDS_CLASS_VENGEANCE };

		if (presetName == "druid_balance") return { CLASS_IDS::CLASS_IDS_DRUID, DRUID_SPEC_IDS::DRUID_SPEC_IDS_BALANCE };
		if (presetName == "druid_class_balance") return { CLASS_IDS::CLASS_IDS_DRUID, DRUID_SPEC_IDS::DRUID_SPEC_IDS_CLASS_BALANCE };
		if (presetName == "druid_feral") return { CLASS_IDS::CLASS_IDS_DRUID, DRUID_SPEC_IDS::DRUID_SPEC_IDS_FERAL};
		if (presetName == "druid_class_feral") return { CLASS_IDS::CLASS_IDS_DRUID, DRUID_SPEC_IDS::DRUID_SPEC_IDS_CLASS_FERAL};
		if (presetName == "druid_guardian") return { CLASS_IDS::CLASS_IDS_DRUID, DRUID_SPEC_IDS::DRUID_SPEC_IDS_GUARDIAN };
		if (presetName == "druid_class_guardian") return { CLASS_IDS::CLASS_IDS_DRUID, DRUID_SPEC_IDS::DRUID_SPEC_IDS_CLASS_GUARDIAN};
		if (presetName == "druid_restoration") return { CLASS_IDS::CLASS_IDS_DRUID, DRUID_SPEC_IDS::DRUID_SPEC_IDS_RESTORATION };
		if (presetName == "druid_class_restoration") return { CLASS_IDS::CLASS_IDS_DRUID, DRUID_SPEC_IDS::DRUID_SPEC_IDS_CLASS_RESTORATION};

		if (presetName == "evoker_devastation") return { CLASS_IDS::CLASS_IDS_EVOKER, EVOKER_SPEC_IDS::EVOKER_SPEC_IDS_DEVASTATION };
		if (presetName == "evoker_class_devastation") return { CLASS_IDS::CLASS_IDS_EVOKER, EVOKER_SPEC_IDS::EVOKER_SPEC_IDS_CLASS_DEVASTATION };
		if (presetName == "evoker_preservation") return { CLASS_IDS::CLASS_IDS_EVOKER, EVOKER_SPEC_IDS::EVOKER_SPEC_IDS_PRESERVATION};
		if (presetName == "evoker_class_preservation") return { CLASS_IDS::CLASS_IDS_EVOKER, EVOKER_SPEC_IDS::EVOKER_SPEC_IDS_CLASS_PRESERVATION};

		if (presetName == "hunter_beastmastery") return { CLASS_IDS::CLASS_IDS_HUNTER, HUNTER_SPEC_IDS::HUNTER_SPEC_IDS_BEASTMASTERY };
		if (presetName == "hunter_class_beastmastery") return { CLASS_IDS::CLASS_IDS_HUNTER, HUNTER_SPEC_IDS::HUNTER_SPEC_IDS_CLASS_BEASTMASTERY };
		if (presetName == "hunter_marksmanship") return { CLASS_IDS::CLASS_IDS_HUNTER, HUNTER_SPEC_IDS::HUNTER_SPEC_IDS_MARKSMANSHIP };
		if (presetName == "hunter_class_marksmanship") return { CLASS_IDS::CLASS_IDS_HUNTER, HUNTER_SPEC_IDS::HUNTER_SPEC_IDS_CLASS_MARKSMANSHIP};
		if (presetName == "hunter_survival") return { CLASS_IDS::CLASS_IDS_HUNTER, HUNTER_SPEC_IDS::HUNTER_SPEC_IDS_SURVIVAL };
		if (presetName == "hunter_class_survival") return { CLASS_IDS::CLASS_IDS_HUNTER, HUNTER_SPEC_IDS::HUNTER_SPEC_IDS_CLASS_SURVIVAL};

		if (presetName == "mage_arcane") return { CLASS_IDS::CLASS_IDS_MAGE, MAGE_SPEC_IDS::MAGE_SPEC_IDS_ARCANE };
		if (presetName == "mage_class_arcane") return { CLASS_IDS::CLASS_IDS_MAGE, MAGE_SPEC_IDS::MAGE_SPEC_IDS_CLASS_ARCANCE };
		if (presetName == "mage_fire") return { CLASS_IDS::CLASS_IDS_MAGE, MAGE_SPEC_IDS::MAGE_SPEC_IDS_FIRE};
		if (presetName == "mage_class_fire") return { CLASS_IDS::CLASS_IDS_MAGE, MAGE_SPEC_IDS::MAGE_SPEC_IDS_CLASS_FIRE};
		if (presetName == "mage_frost") return { CLASS_IDS::CLASS_IDS_MAGE, MAGE_SPEC_IDS::MAGE_SPEC_IDS_FROST};
		if (presetName == "mage_class_frost") return { CLASS_IDS::CLASS_IDS_MAGE, MAGE_SPEC_IDS::MAGE_SPEC_IDS_CLASS_FROST};

		if (presetName == "monk_brewmaster") return { CLASS_IDS::CLASS_IDS_MONK, MONK_SPEC_IDS::MONK_SPEC_IDS_BREWMASTER };
		if (presetName == "monk_class_brewmaster") return { CLASS_IDS::CLASS_IDS_MONK, MONK_SPEC_IDS::MONK_SPEC_IDS_CLASS_BREWMASTER };
		if (presetName == "monk_mistweaver") return { CLASS_IDS::CLASS_IDS_MONK, MONK_SPEC_IDS::MONK_SPEC_IDS_MISTWEAVER};
		if (presetName == "monk_class_mistweaver") return { CLASS_IDS::CLASS_IDS_MONK, MONK_SPEC_IDS::MONK_SPEC_IDS_CLASS_MISTWEAVER};
		if (presetName == "monk_windwalker") return { CLASS_IDS::CLASS_IDS_MONK, MONK_SPEC_IDS::MONK_SPEC_IDS_WINDWALKER};
		if (presetName == "monk_class_windwalker") return { CLASS_IDS::CLASS_IDS_MONK, MONK_SPEC_IDS::MONK_SPEC_IDS_CLASS_WINDWALKER};

		if (presetName == "paladin_holy") return { CLASS_IDS::CLASS_IDS_PALADIN, PALADIN_SPEC_IDS::PALADIN_SPEC_IDS_HOLY };
		if (presetName == "paladin_class_holy") return { CLASS_IDS::CLASS_IDS_PALADIN, PALADIN_SPEC_IDS::PALADIN_SPEC_IDS_CLASS_HOLY };
		if (presetName == "paladin_protection") return { CLASS_IDS::CLASS_IDS_PALADIN, PALADIN_SPEC_IDS::PALADIN_SPEC_IDS_PROTECTION };
		if (presetName == "paladin_class_protection") return { CLASS_IDS::CLASS_IDS_PALADIN, PALADIN_SPEC_IDS::PALADIN_SPEC_IDS_CLASS_PROTECTION};
		if (presetName == "paladin_retribution") return { CLASS_IDS::CLASS_IDS_PALADIN, PALADIN_SPEC_IDS::PALADIN_SPEC_IDS_RETRIBUTION };
		if (presetName == "paladin_class_retribution") return { CLASS_IDS::CLASS_IDS_PALADIN, PALADIN_SPEC_IDS::PALADIN_SPEC_IDS_CLASS_RETRIBUTION};

		if (presetName == "priest_discipline") return { CLASS_IDS::CLASS_IDS_PRIEST, PRIEST_SPEC_IDS::PRIEST_SPEC_IDS_DISCIPLINE };
		if (presetName == "priest_class_discipline") return { CLASS_IDS::CLASS_IDS_PRIEST, PRIEST_SPEC_IDS::PRIEST_SPEC_IDS_CLASS_DISCIPLINE };
		if (presetName == "priest_holy") return { CLASS_IDS::CLASS_IDS_PRIEST, PRIEST_SPEC_IDS::PRIEST_SPEC_IDS_HOLY};
		if (presetName == "priest_class_holy") return { CLASS_IDS::CLASS_IDS_PRIEST, PRIEST_SPEC_IDS::PRIEST_SPEC_IDS_CLASS_HOLY};
		if (presetName == "priest_shadow") return { CLASS_IDS::CLASS_IDS_PRIEST, PRIEST_SPEC_IDS::PRIEST_SPEC_IDS_SHADOW };
		if (presetName == "priest_class_shadow") return { CLASS_IDS::CLASS_IDS_PRIEST, PRIEST_SPEC_IDS::PRIEST_SPEC_IDS_CLASS_SHADOW};

		if (presetName == "rogue_assassination") return { CLASS_IDS::CLASS_IDS_ROGUE, ROGUE_SPEC_IDS::ROGUE_SPEC_IDS_ASSASSINATION };
		if (presetName == "rogue_class_assassination") return { CLASS_IDS::CLASS_IDS_ROGUE, ROGUE_SPEC_IDS::ROGUE_SPEC_IDS_CLASS_ASSASSINATION };
		if (presetName == "rogue_outlaw") return { CLASS_IDS::CLASS_IDS_ROGUE, ROGUE_SPEC_IDS::ROGUE_SPEC_IDS_OUTLAW};
		if (presetName == "rogue_class_outlaw") return { CLASS_IDS::CLASS_IDS_ROGUE, ROGUE_SPEC_IDS::ROGUE_SPEC_IDS_CLASS_OUTLAW};
		if (presetName == "rogue_subtlety") return { CLASS_IDS::CLASS_IDS_ROGUE, ROGUE_SPEC_IDS::ROGUE_SPEC_IDS_SUBTLETY};
		if (presetName == "rogue_class_subtlety") return { CLASS_IDS::CLASS_IDS_ROGUE, ROGUE_SPEC_IDS::ROGUE_SPEC_IDS_CLASS_SUBTLETY};

		if (presetName == "shaman_elemental") return { CLASS_IDS::CLASS_IDS_SHAMAN, SHAMAN_SPEC_IDS::SHAMAN_SPEC_IDS_ELEMENTAL };
		if (presetName == "shaman_class_elemental") return { CLASS_IDS::CLASS_IDS_SHAMAN, SHAMAN_SPEC_IDS::SHAMAN_SPEC_IDS_CLASS_ELEMENTAL };
		if (presetName == "shaman_enhancement") return { CLASS_IDS::CLASS_IDS_SHAMAN, SHAMAN_SPEC_IDS::SHAMAN_SPEC_IDS_ENHANCEMENT};
		if (presetName == "shaman_class_enhancement") return { CLASS_IDS::CLASS_IDS_SHAMAN, SHAMAN_SPEC_IDS::SHAMAN_SPEC_IDS_CLASS_ENHANCEMENT};
		if (presetName == "shaman_restoration") return { CLASS_IDS::CLASS_IDS_SHAMAN, SHAMAN_SPEC_IDS::SHAMAN_SPEC_IDS_RESTORATION};
		if (presetName == "shaman_class_restoration") return { CLASS_IDS::CLASS_IDS_SHAMAN, SHAMAN_SPEC_IDS::SHAMAN_SPEC_IDS_CLASS_RESTORATION};

		if (presetName == "warlock_affliction") return { CLASS_IDS::CLASS_IDS_WARLOCK, WARLOCK_SPEC_IDS::WARLOCK_SPEC_IDS_AFFLICTION };
		if (presetName == "warlock_class_affliction") return { CLASS_IDS::CLASS_IDS_WARLOCK, WARLOCK_SPEC_IDS::WARLOCK_SPEC_IDS_CLASS_AFFLICTION };
		if (presetName == "warlock_demonology") return { CLASS_IDS::CLASS_IDS_WARLOCK, WARLOCK_SPEC_IDS::WARLOCK_SPEC_IDS_DEMONOLOGY};
		if (presetName == "warlock_class_demonology") return { CLASS_IDS::CLASS_IDS_WARLOCK, WARLOCK_SPEC_IDS::WARLOCK_SPEC_IDS_CLASS_DEMONOLOGY};
		if (presetName == "warlock_destruction") return { CLASS_IDS::CLASS_IDS_WARLOCK, WARLOCK_SPEC_IDS::WARLOCK_SPEC_IDS_DESTRUCTION };
		if (presetName == "warlock_class_destruction") return { CLASS_IDS::CLASS_IDS_WARLOCK, WARLOCK_SPEC_IDS::WARLOCK_SPEC_IDS_CLASS_DESTRUCTION};

		if (presetName == "warrior_arms") return { CLASS_IDS::CLASS_IDS_WARRIOR, WARRIOR_SPEC_IDS::WARRIOR_SPEC_IDS_ARMS };
		if (presetName == "warrior_class_arms") return { CLASS_IDS::CLASS_IDS_WARRIOR, WARRIOR_SPEC_IDS::WARRIOR_SPEC_IDS_CLASS_ARMS };
		if (presetName == "warrior_fury") return { CLASS_IDS::CLASS_IDS_WARRIOR, WARRIOR_SPEC_IDS::WARRIOR_SPEC_IDS_FURY};
		if (presetName == "warrior_class_fury") return { CLASS_IDS::CLASS_IDS_WARRIOR, WARRIOR_SPEC_IDS::WARRIOR_SPEC_IDS_CLASS_FURY};
		if (presetName == "warrior_protection") return { CLASS_IDS::CLASS_IDS_WARRIOR, WARRIOR_SPEC_IDS::WARRIOR_SPEC_IDS_PROTECTION };
		if (presetName == "warrior_class_protection") return { CLASS_IDS::CLASS_IDS_WARRIOR, WARRIOR_SPEC_IDS::WARRIOR_SPEC_IDS_CLASS_PROTECTION};



		return { -1, -1 };
	}

	std::filesystem::path getAppPath() {
		std::filesystem::path path;
		PWSTR path_tmp;

		/* Attempt to get user's AppData folder
		 *
		 * This breaks Windows XP and earlier support!
		 *
		 * Microsoft Docs:
		 * https://docs.microsoft.com/en-us/windows/win32/api/shlobj_core/nf-shlobj_core-shgetknownfolderpath
		 * https://docs.microsoft.com/en-us/windows/win32/shell/knownfolderid
		 */
		auto get_folder_path_ret = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path_tmp);

		/* Error check */
		if (get_folder_path_ret != S_OK) {
			CoTaskMemFree(path_tmp);
			throw std::system_error::exception("Could not open/find Roaming App Data path!");
		}

		/* Convert the Windows path type to a C++ path */
		path = path_tmp;
		std::filesystem::path appPath = path / "WoWTalentTreeManager";

		/* Free memory :) */
		CoTaskMemFree(path_tmp);

		//create app folder if it doesn't exist
		if (!std::filesystem::is_directory(appPath)) {
			std::filesystem::create_directory(appPath);
		}

		return appPath;
	}

	std::map<std::string, std::string> LOAD_PRESETS() {
		std::map<std::string, std::string> presets;
		std::filesystem::path presetPath = getAppPath() / "resources" / "presets.txt";
		if (std::filesystem::is_regular_file(presetPath)) {
			std::ifstream treeFile;
			std::string line;
			treeFile.open(presetPath);

			if (treeFile.is_open())
			{
				while (!treeFile.eof())
				{
					std::getline(treeFile, line);
					size_t pos1 = line.find(':') + 1;
					std::string presetName = line.substr(pos1, line.substr(pos1).find(':'));
					if (presetName.length() <= 0) {
						continue;
					}
					presets[presetName] = line;
				}
			}
		}
		else {
			//TTMTODO: Implement error logger for engine too
			std::ofstream presetErrorFile("error_log.txt");
			presetErrorFile << "Preset file was not present and couldn't be updated (maybe no internet connection)\n";
		}

		return presets;
	}

	CLASS_IDS CLASS_ID_FROM_PRESET_NAME(std::string presetName) {
		std::string classIdentifier = presetName.substr(0, presetName.find('_'));
		if (classIdentifier == "deathknight") {
			return CLASS_IDS_DEATHKNIGHT;
		}
		else if (classIdentifier == "demonhunter") {
			return CLASS_IDS_DEMONHUNTER;
		}
		else if (classIdentifier == "druid") {
			return CLASS_IDS_DRUID;
		}
		else if (classIdentifier == "evoker") {
			return CLASS_IDS_EVOKER;
		}
		else if (classIdentifier == "hunter") {
			return CLASS_IDS_HUNTER;
		}
		else if (classIdentifier == "mage") {
			return CLASS_IDS_MAGE;
		}
		else if (classIdentifier == "monk") {
			return CLASS_IDS_MONK;
		}
		else if (classIdentifier == "paladin") {
			return CLASS_IDS_PALADIN;
		}
		else if (classIdentifier == "priest") {
			return CLASS_IDS_PRIEST;
		}
		else if (classIdentifier == "rogue") {
			return CLASS_IDS_ROGUE;
		}
		else if (classIdentifier == "shaman") {
			return CLASS_IDS_SHAMAN;
		}
		else if (classIdentifier == "warlock") {
			return CLASS_IDS_WARLOCK;
		}
		else if (classIdentifier == "warrior") {
			return CLASS_IDS_WARRIOR;
		}
		else {
			return CLASS_IDS_NONE;
		}
	}

	// first two ints are class id and spec id, if vector empty then file read error
	std::string LOAD_RAW_NODE_ID_ORDER(std::string presetName) {
		size_t classPresetNamePos = presetName.find("_class");
		if (classPresetNamePos != std::string::npos) {
			presetName = presetName.substr(0, classPresetNamePos) + presetName.substr(classPresetNamePos + 6);
		}
		bool foundPresetName = false;
		std::filesystem::path nodeIDOrdersPath = getAppPath() / "resources" / "node_id_orders.txt";
		std::string line;
		if (std::filesystem::is_regular_file(nodeIDOrdersPath)) {
			std::ifstream treeFile;
			treeFile.open(nodeIDOrdersPath);

			if (treeFile.is_open())
			{
				while (!treeFile.eof())
				{
					std::getline(treeFile, line);
					if (line.substr(0, line.find(':')) == presetName) {
						foundPresetName = true;
						break;
					}
				}
			}
		}
		else {
			//TTMTODO: Implement error logger for engine too
			std::ofstream presetErrorFile("error_log.txt");
			presetErrorFile << "Preset file was not present and couldn't be updated (maybe no internet connection)\n";
		}

		if (!foundPresetName) {
			return "";
		}

		return line;
	}

	std::string PRESET_NAME_FROM_BLIZZ_SPEC_ID(size_t spec_id) {
		std::string targetPresetName = "";
		std::filesystem::path nodeIDOrdersPath = getAppPath() / "resources" / "node_id_orders.txt";
		std::string line;
		if (std::filesystem::is_regular_file(nodeIDOrdersPath)) {
			std::ifstream treeFile;
			treeFile.open(nodeIDOrdersPath);

			if (treeFile.is_open())
			{
				while (!treeFile.eof())
				{
					std::getline(treeFile, line);
					size_t p1 = line.find(':');
					size_t p2 = line.find(':', p1 + 1);
					size_t p3 = line.find(':', p2 + 1);
					std::string presetName = line.substr(0, p1);
					std::string blizzClassIDStr = line.substr(p1 + 1, p2 - p1 - 1);
					std::string blizzSpecIDStr = line.substr(p2 + 1, p3 - p2 - 1);
					if (std::atoi(blizzSpecIDStr.c_str()) == static_cast<int>(spec_id)) {
						targetPresetName = presetName;
						treeFile.close();
						break;
					}
				}
			}
		}
		else {
			//TTMTODO: Implement error logger for engine too
			std::ofstream presetErrorFile("error_log.txt");
			presetErrorFile << "Preset file was not present and couldn't be updated (maybe no internet connection)\n";
			return "";
		}
		return targetPresetName;
	}

	std::pair<std::string, std::string> CLASS_SPEC_PRESETS_FROM_BLIZZ_SPEC_ID(std::map<std::string, std::string> presets, size_t spec_id) {
		bool foundPreset = false;
		std::string targetPresetName = PRESET_NAME_FROM_BLIZZ_SPEC_ID(spec_id);
		foundPreset = targetPresetName != "";
		if (!foundPreset) {
			return { "", ""};
		}

		size_t sep = targetPresetName.find('_');
		std::string targetPresetNameClass =
			targetPresetName.substr(0, sep)
			+ "_class"
			+ targetPresetName.substr(sep);
		
		if (presets.count(targetPresetNameClass) && presets.count(targetPresetName)) {
			return { presets[targetPresetNameClass], presets[targetPresetName] };
		}
		else {
			return { "", "" };
		}
	}
}