#pragma once

#include <string>
#include "TalentTrees.h"
#include "TreeSolver.h"

namespace CLI {
	struct CLSettings {
		bool missingStructureFilePath = false;
		bool filterProvided = false;
		bool generateOutput = false;
		std::string structureFilePath;
		std::string rawStructureIndices;
		std::string filterFilePath;
		std::string rawFilter;
		std::string outputFilePath;
		int targetTalentCount = 1;
		bool solveParallel = false;
	};

	struct RunDetails {
		Engine::TalentTree tree;
		std::shared_ptr<Engine::TreeDAGInfo> treeDAGInfo;
		int targetTalentCount = 1;
		std::shared_ptr<Engine::TalentSkillset> filter;
		Engine::SIND excludeFilter = 0;
		bool safetyGuardTriggered = false;
		std::vector<int> bitToIndexVec;
		std::vector<std::vector<int>> assignedSwitchIndices;
	};

	CLSettings processCommandLine(int argc, char** argv);
	void runCombinationCount(CLSettings settings);
	void printSettings(CLSettings settings);
	std::vector<RunDetails> generateRunDetails(CLSettings settings);
	void startThreadedCombinationCount(std::vector<RunDetails>& allRunDetails, CLSettings& settings);
	void outputCombinations(std::vector<RunDetails>& allRunDetails, CLSettings& settings);
}