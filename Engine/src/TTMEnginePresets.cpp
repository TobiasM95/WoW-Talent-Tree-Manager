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
		case CLASS_IDS_DEATHKNIGHT: return 4;
		case CLASS_IDS_DEMONHUNTER: return 3;
		case CLASS_IDS_DRUID: return 5;
		case CLASS_IDS_EVOKER: return 3;
		case CLASS_IDS_HUNTER: return 4;
		case CLASS_IDS_MAGE: return 4;
		case CLASS_IDS_MONK: return 4;
		case CLASS_IDS_PALADIN: return 4;
		case CLASS_IDS_PRIEST: return 4;
		case CLASS_IDS_ROGUE: return 4;
		case CLASS_IDS_SHAMAN: return 4;
		case CLASS_IDS_WARLOCK: return 4;
		case CLASS_IDS_WARRIOR: return 4;
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
			case DEATHKNIGHT_SPEC_IDS_CLASS: return presets["deathknight_class"];
			}
		}
		case CLASS_IDS_DEMONHUNTER: {
			switch (specID) {
			case DEMONHUNTER_SPEC_IDS_HAVOC: return presets["demonhunter_havoc"];
			case DEMONHUNTER_SPEC_IDS_VENGEANCE: return presets["demonhunter_vengeance"];
			case DEMONHUNTER_SPEC_IDS_CLASS: return presets["demonhunter_class"];
			}
		}
		case CLASS_IDS_DRUID: {
			switch (specID) {
			case DRUID_SPEC_IDS_BALANCE: return presets["druid_balance"];
			case DRUID_SPEC_IDS_FERAL: return presets["druid_feral"];
			case DRUID_SPEC_IDS_GUARDIAN: return presets["druid_guardian"];
			case DRUID_SPEC_IDS_RESTORATION: return presets["druid_restoration"];
			case DRUID_SPEC_IDS_CLASS: return presets["druid_class"];
			}
		}
		case CLASS_IDS_EVOKER: {
			switch (specID) {
			case EVOKER_SPEC_IDS_DEVASTATION: return presets["evoker_devastation"];
			case EVOKER_SPEC_IDS_PRESERVATION: return presets["evoker_preservation"];
			case EVOKER_SPEC_IDS_CLASS: return presets["evoker_class"];
			}
		}
		case CLASS_IDS_HUNTER: {
			switch (specID) {
			case HUNTER_SPEC_IDS_BEASTMASTERY: return presets["hunter_beastmastery"];
			case HUNTER_SPEC_IDS_MARKSMANSHIP: return presets["hunter_marksmanship"];
			case HUNTER_SPEC_IDS_SURVIVAL: return presets["hunter_survival"];
			case HUNTER_SPEC_IDS_CLASS: return presets["hunter_class"];
			}
		}
		case CLASS_IDS_MAGE: {
			switch (specID) {
			case MAGE_SPEC_IDS_ARCANE: return presets["mage_arcance"];
			case MAGE_SPEC_IDS_FIRE: return presets["mage_fire"];
			case MAGE_SPEC_IDS_FROST: return presets["mage_frost"];
			case MAGE_SPEC_IDS_CLASS: return presets["mage_class"];
			}
		}
		case CLASS_IDS_MONK: {
			switch (specID) {
			case MONK_SPEC_IDS_BREWMASTER: return presets["monk_brewmaster"];
			case MONK_SPEC_IDS_MISTWEAVER: return presets["monk_mistweaver"];
			case MONK_SPEC_IDS_WINDWALKER: return presets["monk_windwalker"];
			case MONK_SPEC_IDS_CLASS: return presets["monk_class"];
			}
		}
		case CLASS_IDS_PALADIN: {
			switch (specID) {
			case PALADIN_SPEC_IDS_HOLY: return presets["paladin_holy"];
			case PALADIN_SPEC_IDS_PROTECTION: return presets["paladin_protection"];
			case PALADIN_SPEC_IDS_RETRIBUTION: return presets["paladin_retribution"];
			case PALADIN_SPEC_IDS_CLASS: return presets["paladin_class"];
			}
		}
		case CLASS_IDS_PRIEST: {
			switch (specID) {
			case PRIEST_SPEC_IDS_DISCIPLINE: return presets["priest_discipline"];
			case PRIEST_SPEC_IDS_HOLY: return presets["priest_holy"];
			case PRIEST_SPEC_IDS_SHADOW: return presets["priest_shadow"];
			case PRIEST_SPEC_IDS_CLASS: return presets["priest_class"];
			}
		}
		case CLASS_IDS_ROGUE: {
			switch (specID) {
			case ROGUE_SPEC_IDS_ASSASSINATION: return presets["rogue_assassination"];
			case ROGUE_SPEC_IDS_OUTLAW: return presets["rogue_outlaw"];
			case ROGUE_SPEC_IDS_SUBTLETY: return presets["rogue_subtlety"];
			case ROGUE_SPEC_IDS_CLASS: return presets["rogue_class"];
			}
		}
		case CLASS_IDS_SHAMAN: {
			switch (specID) {
			case SHAMAN_SPEC_IDS_ELEMENTAL: return presets["shaman_elemental"];
			case SHAMAN_SPEC_IDS_ENHANCEMENT: return presets["shaman_enhancement"];
			case SHAMAN_SPEC_IDS_RESTORATION: return presets["shaman_restoration"];
			case SHAMAN_SPEC_IDS_CLASS: return presets["shaman_class"];
			}
		}
		case CLASS_IDS_WARLOCK: {
			switch (specID) {
			case WARLOCK_SPEC_IDS_AFFLICTION: return presets["warlock_affliction"];
			case WARLOCK_SPEC_IDS_DEMONOLOGY: return presets["warlock_demonology"];
			case WARLOCK_SPEC_IDS_DESTRUCTION: return presets["warlock_destruction"];
			case WARLOCK_SPEC_IDS_CLASS: return presets["warlock_class"];
			}
		}
		case CLASS_IDS_WARRIOR: {
			switch (specID) {
			case WARRIOR_SPEC_IDS_ARMS: return presets["warrior_arms"];
			case WARRIOR_SPEC_IDS_FURY: return presets["warrior_fury"];
			case WARRIOR_SPEC_IDS_PROTECTION: return presets["warrior_protection"];
			case WARRIOR_SPEC_IDS_CLASS: return presets["warrior_class"];
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
					std::string presetName = line.substr(0, line.find(':'));
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