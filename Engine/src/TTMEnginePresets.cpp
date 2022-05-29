/*
	WoW Talent Tree Manager is an application for creating/editing/sharing talent trees and setups.
	Copyright(C) 2022 TobiasM95

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
		case CLASS_IDS_DRUID: return 2;
		case CLASS_IDS_EVOKER: return 1;
		default: throw std::logic_error("Class ID does not exist!");
		}
	}

	const char** RETURN_SPECS(int classID) {
		switch (classID) {
		case CLASS_IDS_DRUID: return SPECS_DRUID;
		case CLASS_IDS_EVOKER: return SPECS_EVOKER;
		default: throw std::logic_error("Class ID does not exist!");
		}
	}

	const std::string RETURN_PRESET(int classID, int specID) {
		switch (classID) {
		case CLASS_IDS_DRUID: {
			switch (specID) {
			case DRUID_SPEC_IDS_RESTORATION: return DRUID_RESTORATION_PRESET;
			case DRUID_SPEC_IDS_CLASSIC: return DRUID_CLASSIC_PRESET;
			}
		}
		case CLASS_IDS_EVOKER: {
			switch (specID) {
			case EVOKER_SPEC_IDS_DEVASTATION: return EVOKER_DEVASTATION_PRESET;
			}
		}
		default: throw std::logic_error("Combination of class ID and spec ID does not exist!");
		}
	}

	const std::string RETURN_PRESET_BY_NAME(std::string presetNAME) {
		if (presetNAME == "new_custom_tree") return NEW_CUSTOM_TREE_PRESET;
		if (presetNAME == "druid_restoration") return RETURN_PRESET(CLASS_IDS_DRUID, DRUID_SPEC_IDS_RESTORATION);
		if (presetNAME == "druid_classic") return RETURN_PRESET(CLASS_IDS_DRUID, DRUID_SPEC_IDS_CLASSIC);
		if (presetNAME == "evoker_devastation") return RETURN_PRESET(CLASS_IDS_EVOKER, EVOKER_SPEC_IDS_DEVASTATION);
		throw std::logic_error("Preset name not found!");
	}
}