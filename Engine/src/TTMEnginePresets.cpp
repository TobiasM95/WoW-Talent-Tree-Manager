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