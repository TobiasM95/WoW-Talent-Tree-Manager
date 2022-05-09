#include "TTMEnginePresets.h"

namespace Presets {

	const char** RETURN_SPECS(int classID) {
		switch (classID) {
		case CLASS_IDS_DRUID: return SPECS_DRUID;
		case CLASS_IDS_EVOKER: return SPECS_EVOKER;
		}
	}

	const std::string RETURN_PRESET(int classID, int specID) {
		switch (classID) {
		case CLASS_IDS_DRUID: {
			switch (specID) {
			case DRUID_SPEC_IDS_RESTORATION: return DRUID_RESTORATION_PRESET;
			}
		}
		case CLASS_IDS_EVOKER: {
			switch (specID) {
			case EVOKER_SPEC_IDS_DEVASTATION: return EVOKER_DEVASTATION_PRESET;
			}
		}
		}
	}

	const std::string RETURN_PRESET_BY_NAME(std::string presetNAME) {
		if (presetNAME == "druid_restoration") return RETURN_PRESET(CLASS_IDS_DRUID, DRUID_SPEC_IDS_RESTORATION);
		if (presetNAME == "evoker_devastation") return RETURN_PRESET(CLASS_IDS_EVOKER, EVOKER_SPEC_IDS_DEVASTATION);
	}
}