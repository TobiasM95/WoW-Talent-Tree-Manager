#pragma once

namespace CLI {
	struct CLSettings {

	};

	CLSettings processCommandLine(int argc, char** argv);
	void runCombinationCount(CLSettings settings);
}