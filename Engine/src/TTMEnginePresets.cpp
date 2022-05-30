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

#include <stdexcept>

namespace Presets {

	const int RETURN_SPEC_COUNT(int classID) {
		switch (classID) {
		case CLASS_IDS_DEATHKNIGHT: return 3;
		case CLASS_IDS_DEMONHUNTER: return 2;
		case CLASS_IDS_DRUID: return 5;
		case CLASS_IDS_EVOKER: return 2;
		case CLASS_IDS_HUNTER: return 3;
		case CLASS_IDS_MAGE: return 3;
		case CLASS_IDS_MONK: return 3;
		case CLASS_IDS_PALADIN: return 3;
		case CLASS_IDS_PRIEST: return 3;
		case CLASS_IDS_ROGUE: return 3;
		case CLASS_IDS_SHAMAN: return 3;
		case CLASS_IDS_WARLOCK: return 3;
		case CLASS_IDS_WARRIOR: return 3;
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

	const std::string RETURN_PRESET(int classID, int specID) {
		switch (classID) {
		case CLASS_IDS_DEATHKNIGHT: {
			switch (specID) {
			case DEATHKNIGHT_SPEC_IDS_BLOOD: return DEATHKNIGHT_BLOOD_PRESET;
			case DEATHKNIGHT_SPEC_IDS_FROST: return DEATHKNIGHT_FROST_PRESET;
			case DEATHKNIGHT_SPEC_IDS_UNHOLY: return DEATHKNIGHT_UNHOLY_PRESET;
			}
		}
		case CLASS_IDS_DEMONHUNTER: {
			switch (specID) {
			case DEMONHUNTER_SPEC_IDS_HAVOC: return DEMONHUNTER_HAVOC_PRESET;
			case DEMONHUNTER_SPEC_IDS_VENGEANCE: return DEMONHUNTER_VENGEANCE_PRESET;
			}
		}
		case CLASS_IDS_DRUID: {
			switch (specID) {
			case DRUID_SPEC_IDS_BALANCE: return DRUID_BALANCE_PRESET;
			case DRUID_SPEC_IDS_FERAL: return DRUID_FERAL_PRESET;
			case DRUID_SPEC_IDS_GUARDIAN: return DRUID_GUARDIAN_PRESET;
			case DRUID_SPEC_IDS_RESTORATION: return DRUID_RESTORATION_PRESET;
			case DRUID_SPEC_IDS_CLASSIC: return DRUID_CLASSIC_PRESET;
			}
		}
		case CLASS_IDS_EVOKER: {
			switch (specID) {
			case EVOKER_SPEC_IDS_DEVASTATION: return EVOKER_DEVASTATION_PRESET;
			case EVOKER_SPEC_IDS_PRESERVATION: return EVOKER_PRESERVATION_PRESET;
			}
		}
		case CLASS_IDS_HUNTER: {
			switch (specID) {
			case HUNTER_SPEC_IDS_BEASTMASTERY: return HUNTER_BEASTMASTERY_PRESET;
			case HUNTER_SPEC_IDS_MARKSMANSHIP: return HUNTER_MARKSMANSHIP_PRESET;
			case HUNTER_SPEC_IDS_SURVIVAL: return HUNTER_SURVIVAL_PRESET;
			}
		}
		case CLASS_IDS_MAGE: {
			switch (specID) {
			case MAGE_SPEC_IDS_ARCANE: return MAGE_ARCANE_PRESET;
			case MAGE_SPEC_IDS_FIRE: return MAGE_FIRE_PRESET;
			case MAGE_SPEC_IDS_FROST: return MAGE_FROST_PRESET;
			}
		}
		case CLASS_IDS_MONK: {
			switch (specID) {
			case MONK_SPEC_IDS_BREWMASTER: return MONK_BREWMASTER_PRESET;
			case MONK_SPEC_IDS_MISTWEAVER: return MONK_MISTWEAVER_PRESET;
			case MONK_SPEC_IDS_WINDWALKER: return MONK_WINDWALKER_PRESET;
			}
		}
		case CLASS_IDS_PALADIN: {
			switch (specID) {
			case PALADIN_SPEC_IDS_HOLY: return PALADIN_HOLY_PRESET;
			case PALADIN_SPEC_IDS_PROTECTION: return PALADIN_PROTECTION_PRESET;
			case PALADIN_SPEC_IDS_RETRIBUTION: return PALADIN_RETRIBUTION_PRESET;
			}
		}
		case CLASS_IDS_PRIEST: {
			switch (specID) {
			case PRIEST_SPEC_IDS_DISCIPLINE: return PRIEST_DISCIPLINE_PRESET;
			case PRIEST_SPEC_IDS_HOLY: return PRIEST_HOLY_PRESET;
			case PRIEST_SPEC_IDS_SHADOW: return PRIEST_SHADOW_PRESET;
			}
		}
		case CLASS_IDS_ROGUE: {
			switch (specID) {
			case ROGUE_SPEC_IDS_ASSASSINATION: return ROGUE_ASSASSINATION_PRESET;
			case ROGUE_SPEC_IDS_OUTLAW: return ROGUE_OUTLAW_PRESET;
			case ROGUE_SPEC_IDS_SUBTLETY: return ROGUE_SUBTLETY_PRESET;
			}
		}
		case CLASS_IDS_SHAMAN: {
			switch (specID) {
			case SHAMAN_SPEC_IDS_ELEMENTAL: return SHAMAN_ELEMENTAL_PRESET;
			case SHAMAN_SPEC_IDS_ENHANCEMENT: return SHAMAN_ENHANCEMENT_PRESET;
			case SHAMAN_SPEC_IDS_RESTORATION: return SHAMAN_RESTORATION_PRESET;
			}
		}
		case CLASS_IDS_WARLOCK: {
			switch (specID) {
			case WARLOCK_SPEC_IDS_AFFLICTION: return WARLOCK_AFFLICTION_PRESET;
			case WARLOCK_SPEC_IDS_DEMONOLOGY: return WARLOCK_DEMONOLOGY_PRESET;
			case WARLOCK_SPEC_IDS_DESTRUCTION: return WARLOCK_DESTRUCTION_PRESET;
			}
		}
		case CLASS_IDS_WARRIOR: {
			switch (specID) {
			case WARRIOR_SPEC_IDS_ARMS: return WARRIOR_ARMS_PRESET;
			case WARRIOR_SPEC_IDS_FURY: return WARRIOR_FURY_PRESET;
			case WARRIOR_SPEC_IDS_PROTECTION: return WARRIOR_PROTECTION_PRESET;
			}
		}
		default: throw std::logic_error("Combination of class ID and spec ID does not exist!");
		}
	}

	const std::string RETURN_PRESET_BY_NAME(std::string presetNAME) {
		if (presetNAME == "new_custom_tree") return NEW_CUSTOM_TREE_PRESET;
		if (presetNAME == "deathknight_blood") return RETURN_PRESET(CLASS_IDS_DEATHKNIGHT, DEATHKNIGHT_SPEC_IDS_BLOOD);
		if (presetNAME == "deathknight_frost") return RETURN_PRESET(CLASS_IDS_DEATHKNIGHT, DEATHKNIGHT_SPEC_IDS_FROST);
		if (presetNAME == "deathknight_unholy") return RETURN_PRESET(CLASS_IDS_DEATHKNIGHT, DEATHKNIGHT_SPEC_IDS_UNHOLY);
		if (presetNAME == "demonhunter_havoc") return RETURN_PRESET(CLASS_IDS_DEMONHUNTER, DEMONHUNTER_SPEC_IDS_HAVOC);
		if (presetNAME == "demonhunter_vengeance") return RETURN_PRESET(CLASS_IDS_DEMONHUNTER, DEMONHUNTER_SPEC_IDS_VENGEANCE);
		if (presetNAME == "druid_balance") return RETURN_PRESET(CLASS_IDS_DRUID, DRUID_SPEC_IDS_BALANCE);
		if (presetNAME == "druid_feral") return RETURN_PRESET(CLASS_IDS_DRUID, DRUID_SPEC_IDS_FERAL);
		if (presetNAME == "druid_guardian") return RETURN_PRESET(CLASS_IDS_DRUID, DRUID_SPEC_IDS_GUARDIAN);
		if (presetNAME == "druid_restoration") return RETURN_PRESET(CLASS_IDS_DRUID, DRUID_SPEC_IDS_RESTORATION);
		if (presetNAME == "druid_classic") return RETURN_PRESET(CLASS_IDS_DRUID, DRUID_SPEC_IDS_CLASSIC);
		if (presetNAME == "evoker_devastation") return RETURN_PRESET(CLASS_IDS_EVOKER, EVOKER_SPEC_IDS_DEVASTATION);
		if (presetNAME == "evoker_preservation") return RETURN_PRESET(CLASS_IDS_EVOKER, EVOKER_SPEC_IDS_PRESERVATION);
		if (presetNAME == "hunter_beastmastery") return RETURN_PRESET(CLASS_IDS_HUNTER, HUNTER_SPEC_IDS_BEASTMASTERY);
		if (presetNAME == "hunter_marksmanship") return RETURN_PRESET(CLASS_IDS_HUNTER, HUNTER_SPEC_IDS_MARKSMANSHIP);
		if (presetNAME == "hunter_survival") return RETURN_PRESET(CLASS_IDS_HUNTER, HUNTER_SPEC_IDS_SURVIVAL);
		if (presetNAME == "mage_arcane") return RETURN_PRESET(CLASS_IDS_MAGE, MAGE_SPEC_IDS_ARCANE);
		if (presetNAME == "mage_fire") return RETURN_PRESET(CLASS_IDS_MAGE, MAGE_SPEC_IDS_FIRE);
		if (presetNAME == "mage_frost") return RETURN_PRESET(CLASS_IDS_MAGE, MAGE_SPEC_IDS_FROST);
		if (presetNAME == "monk_brewmaster") return RETURN_PRESET(CLASS_IDS_MONK, MONK_SPEC_IDS_BREWMASTER);
		if (presetNAME == "monk_mistweaver") return RETURN_PRESET(CLASS_IDS_MONK, MONK_SPEC_IDS_MISTWEAVER);
		if (presetNAME == "monk_windwalker") return RETURN_PRESET(CLASS_IDS_MONK, MONK_SPEC_IDS_WINDWALKER);
		if (presetNAME == "paladin_holy") return RETURN_PRESET(CLASS_IDS_PALADIN, PALADIN_SPEC_IDS_HOLY);
		if (presetNAME == "paladin_protection") return RETURN_PRESET(CLASS_IDS_PALADIN, PALADIN_SPEC_IDS_PROTECTION);
		if (presetNAME == "paladin_retribution") return RETURN_PRESET(CLASS_IDS_PALADIN, PALADIN_SPEC_IDS_RETRIBUTION);
		if (presetNAME == "priest_discipline") return RETURN_PRESET(CLASS_IDS_PRIEST, PRIEST_SPEC_IDS_DISCIPLINE);
		if (presetNAME == "priest_holy") return RETURN_PRESET(CLASS_IDS_PRIEST, PRIEST_SPEC_IDS_HOLY);
		if (presetNAME == "priest_shadow") return RETURN_PRESET(CLASS_IDS_PRIEST, PRIEST_SPEC_IDS_SHADOW);
		if (presetNAME == "rogue_assassination") return RETURN_PRESET(CLASS_IDS_ROGUE, ROGUE_SPEC_IDS_ASSASSINATION);
		if (presetNAME == "rogue_outlaw") return RETURN_PRESET(CLASS_IDS_ROGUE, ROGUE_SPEC_IDS_OUTLAW);
		if (presetNAME == "rogue_subtlety") return RETURN_PRESET(CLASS_IDS_ROGUE, ROGUE_SPEC_IDS_SUBTLETY);
		if (presetNAME == "shaman_elemental") return RETURN_PRESET(CLASS_IDS_SHAMAN, SHAMAN_SPEC_IDS_ELEMENTAL);
		if (presetNAME == "shaman_enhancement") return RETURN_PRESET(CLASS_IDS_SHAMAN, SHAMAN_SPEC_IDS_ENHANCEMENT);
		if (presetNAME == "shaman_restoration") return RETURN_PRESET(CLASS_IDS_SHAMAN, SHAMAN_SPEC_IDS_RESTORATION);
		if (presetNAME == "warlock_affliction") return RETURN_PRESET(CLASS_IDS_WARLOCK, WARLOCK_SPEC_IDS_AFFLICTION);
		if (presetNAME == "warlock_demonology") return RETURN_PRESET(CLASS_IDS_WARLOCK, WARLOCK_SPEC_IDS_DEMONOLOGY);
		if (presetNAME == "warlock_destruction") return RETURN_PRESET(CLASS_IDS_WARLOCK, WARLOCK_SPEC_IDS_DESTRUCTION);
		if (presetNAME == "warrior_arms") return RETURN_PRESET(CLASS_IDS_WARRIOR, WARRIOR_SPEC_IDS_ARMS);
		if (presetNAME == "warrior_fury") return RETURN_PRESET(CLASS_IDS_WARRIOR, WARRIOR_SPEC_IDS_FURY);
		if (presetNAME == "warrior_protection") return RETURN_PRESET(CLASS_IDS_WARRIOR, WARRIOR_SPEC_IDS_PROTECTION);
		throw std::logic_error("Preset name not found!");
	}
}