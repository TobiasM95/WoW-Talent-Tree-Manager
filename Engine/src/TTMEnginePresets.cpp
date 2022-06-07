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

#include <filesystem>
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
			case MAGE_SPEC_IDS_ARCANE: return presets["mage_arcance"];
			case MAGE_SPEC_IDS_FIRE: return presets["mage_fire"];
			case MAGE_SPEC_IDS_FROST: return presets["mage_frost"];
			case MAGE_SPEC_IDS_CLASS_ARCANCE: return presets["mage_class_arcance"];
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

	std::map<std::string, std::string> LOAD_PRESETS() {
		std::map<std::string, std::string> presets;
		std::filesystem::path presetPath = std::filesystem::current_path() / "resources" / "presets.txt";
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
}