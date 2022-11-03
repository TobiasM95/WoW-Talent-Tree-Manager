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

#include "CLI.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <ppl.h>

int main(int argc, char** argv)
{
    CLI::CLSettings settings = CLI::processCommandLine(argc, argv);
    if (settings.missingStructureFilePath) {
        std::cout << "Structure file path input is missing! Abort.\n";
        return 1;
    }
    CLI::runCombinationCount(settings);

    return 0;
}

namespace CLI {
    CLSettings processCommandLine(int argc, char** argv) {
        CLSettings settings;

        if (argc <= 1) {
            settings.missingStructureFilePath = true;
            return settings;
        }

        for (int i = 1; i < argc; i++) {
            std::string_view component = std::string_view{ argv[i] };
            if (component == "--structure-file-path" && argc >= i + 1) {
                settings.structureFilePath = std::string{ argv[i + 1] };
            }
            if (component == "--structure-indices" && argc >= i + 1) {
                settings.rawStructureIndices = std::string{ argv[i + 1] };
            }
            if (component == "--filter-file-path" && argc >= i + 1) {
                settings.filterProvided = true;
                settings.filterFilePath = std::string{ argv[i + 1] };
            }
            if (component == "--filter" && argc >= i + 1) {
                settings.filterProvided = true;
                settings.rawFilter = std::string{ argv[i + 1] };
            }
            if (component == "--output-file-path" && argc >= i + 1) {
                settings.generateOutput = true;
                settings.outputFilePath = std::string{ argv[i + 1] };
            }
            if (component == "--target-talent-count" && argc >= i + 1) {
                settings.targetTalentCount = std::stoi(std::string{ argv[i + 1] });
            }
            if (component == "--parallel" || component == "--concurrent") {
                settings.solveParallel = true;
            }
        }

        return settings;
    }

    void runCombinationCount(CLSettings settings) {
        printSettings(settings);
        std::vector<RunDetails> allRunDetails = generateRunDetails(settings);
        if (allRunDetails.size() == 0) {
            std::cout << "No valid trees found in input or input file is corrupt.\n";
            return;
        }
        startThreadedCombinationCount(allRunDetails, settings);
        outputCombinations(allRunDetails, settings);
    }

    void printSettings(CLSettings settings) {
        std::cout << "Settings for the combination count:\n";
        std::cout << "Structure file path:\t" << settings.structureFilePath << "\n";
        std::cout << "Raw structure indices:\t" << settings.rawStructureIndices << "\n";
        if (settings.filterProvided) {
            std::cout << "Filter file path:\t" << settings.filterFilePath << "\n";
            std::cout << "Raw filter input:\t" << settings.rawFilter << "\n";
        }
        else {
            std::cout << "No filter provided.\n";
        }
        if (settings.generateOutput) {
            std::cout << "Output file path:\t" << settings.outputFilePath << "\n";
        }
        else {
            std::cout << "No file output will be generated.\n";
        }
        std::cout << "Target talent count:\t" << settings.targetTalentCount << "\n";
    }

    std::vector<RunDetails> generateRunDetails(CLSettings settings) {
        std::map<size_t, RunDetails> allRunDetailsMap;
        std::vector<RunDetails> allRunDetails;
        std::vector<size_t> selectedStructures;
        for (auto& indexStr : Engine::splitString(settings.rawStructureIndices, ",")) {
            selectedStructures.push_back(static_cast<size_t>(std::stoi(indexStr)));
        }

        // first parse trees to get tree information and run count
        size_t currentStructureIndex = 0;
        if (std::filesystem::is_regular_file(settings.structureFilePath)) {
            std::ifstream structureFile(settings.structureFilePath);
            std::string line;

            while (std::getline(structureFile, line)) {
                if (selectedStructures.size() > 0 
                    && std::find(selectedStructures.begin(), selectedStructures.end(), currentStructureIndex) == selectedStructures.end()) {
                    currentStructureIndex++;
                    continue;
                }
                std::stringstream treeRep{ "" };
                treeRep << line.substr(0, line.find(":"));
                treeRep << ":custom:";
                treeRep << line.substr(line.substr(line.find(":") + 1).find(":") + line.find(":") + 2);

                if (Engine::validateAndRepairTreeStringFormat(treeRep.str())) {
                    RunDetails details;
                    details.tree = Engine::parseTree(treeRep.str());
                    details.targetTalentCount = settings.targetTalentCount;
                    allRunDetailsMap[currentStructureIndex] = details;
                }
                currentStructureIndex++;
            }
        }
        if (selectedStructures.size() > 0) {
            for (size_t& index : selectedStructures) {
                if (allRunDetailsMap.count(index)) {
                    allRunDetails.push_back(allRunDetailsMap[index]);
                }
            }
        }
        else {
            for (auto& indexDetailPair : allRunDetailsMap) {
                allRunDetails.push_back(indexDetailPair.second);
            }
        }

        // check if filter provided, if not, update skillsets to empty, if yes, set up filter skillsets
        if (settings.filterProvided) {
            if (settings.rawFilter != "") {
                std::vector<std::string> filters = Engine::splitString(settings.rawFilter, ";");
                if (filters.back() == "") {
                    filters.pop_back();
                }
                for (size_t i = 0; i < allRunDetails.size(); i++) {
                    if (i >= filters.size()) {
                        break;
                    }
                    std::vector<std::string> filterParts = Engine::splitString(filters[i], ":");
                    if (filterParts.size() - 1 != allRunDetails[i].tree.orderedTalents.size()) {
                        continue;
                    }
                    Engine::TalentSkillset skillset;
                    int skillsetPartIndex = 1;
                    for (auto& indexTalentPair : allRunDetails[i].tree.orderedTalents) {
                        auto& skillsetPart = filterParts[skillsetPartIndex];
                        skillset.assignedSkillPoints[indexTalentPair.first] = std::stoi(skillsetPart);
                        skillsetPartIndex++;
                    }
                    allRunDetails[i].filter = std::make_shared<Engine::TalentSkillset>(skillset);
                }
            }
            else {
                if (std::filesystem::is_regular_file(settings.filterFilePath)) {
                    std::ifstream structureFile(settings.filterFilePath);
                    std::string line;
                    int treeIndex = 0;

                    while (std::getline(structureFile, line)) {
                        if (treeIndex >= allRunDetails.size()) {
                            break;
                        }

                        if (line[line.size() - 1] == ';') {
                            line = line.substr(0, line.size() - 1);
                        }

                        std::vector<std::string> skillsetParts = Engine::splitString(line, ":");

                        //we can ignore the name + meta info at index 0

                        if (skillsetParts.size() - 1 != allRunDetails[treeIndex].tree.orderedTalents.size()) {
                            treeIndex++;
                            continue;
                        }

                        Engine::TalentSkillset skillset;
                        int skillsetPartIndex = 1;
                        for (auto& indexTalentPair : allRunDetails[treeIndex].tree.orderedTalents) {
                            auto& skillsetPart = skillsetParts[skillsetPartIndex];
                            skillset.assignedSkillPoints[indexTalentPair.first] = std::stoi(skillsetPart);
                            skillsetPartIndex++;
                        }

                        allRunDetails[treeIndex].filter = std::make_shared<Engine::TalentSkillset>(skillset);

                        treeIndex++;
                    }
                }
            }
        }
        return allRunDetails;
    }

    void startThreadedCombinationCount(std::vector<RunDetails>& allRunDetails, CLSettings& settings) {
        if (settings.solveParallel) {
            Concurrency::parallel_for(size_t(0), allRunDetails.size(), [&](size_t i) {
                bool dummyProgress = true;
                Engine::clearTree(allRunDetails[i].tree);
                Engine::countConfigurationsFiltered(
                    allRunDetails[i].tree,
                    allRunDetails[i].filter,
                    allRunDetails[i].targetTalentCount,
                    allRunDetails[i].treeDAGInfo,
                    dummyProgress,
                    allRunDetails[i].safetyGuardTriggered
                );
                });
        }
        else {
            for (size_t i = 0; i < allRunDetails.size(); i++) {
                bool dummyProgress = true;
                Engine::clearTree(allRunDetails[i].tree);
                Engine::countConfigurationsFiltered(
                    allRunDetails[i].tree,
                    allRunDetails[i].filter,
                    allRunDetails[i].targetTalentCount,
                    allRunDetails[i].treeDAGInfo,
                    dummyProgress,
                    allRunDetails[i].safetyGuardTriggered
                );
            }
        }

        //generate bit to talent index table
        for (size_t i = 0; i < allRunDetails.size(); i++) {

            std::map<int, int> expandedToCompactIndexMap;
            for (auto& talent : allRunDetails[i].tree.orderedTalents) {
                for (int j = 0; j < talent.second->maxPoints; j++) {
                    //this indexing is taken from TalentTrees.cpp expand talent tree routine and represents an arbitrary indexing
                    //for multipoint talents that doesn't collide for a use in a map
                    //TTMNOTE: If this changes, also change TalentTrees.cpp->expandTalentAndAdvance and TreeSolver.cpp->filterSolvedSkillsets
                    if (j == 0) {
                        expandedToCompactIndexMap[talent.second->index] = talent.second->index;
                    }
                    else {
                        expandedToCompactIndexMap[(talent.second->index + 1) * allRunDetails[i].tree.maxTalentPoints + (j - 1)] = talent.second->index;
                    }
                }
            }

            // normally, TTM indices will already be in that order but since arbitrary
            // unique indices are allowed we're gonna make sure to adhere to this ordering now
            Engine::TalentVec resortedTalents;
            resortedTalents.reserve(allRunDetails[i].treeDAGInfo->sortedTalents.size());
            for (auto& talent : allRunDetails[i].treeDAGInfo->sortedTalents) {
                resortedTalents.push_back(talent);
            }
            std::sort(resortedTalents.begin(), resortedTalents.end(),
                [](const Engine::Talent_s& a, const Engine::Talent_s& b) -> bool
                {
                    if (a->row != b->row) {
                        return a->row < b->row;
                    }
                    return a->column < b->column;
                });

            std::map<int, int> compactToPositionalIndexMap;
            int positionalIndex = 0;
            for (int j = 0; j < static_cast<int>(resortedTalents.size()); j++) {
                int compactIndex = expandedToCompactIndexMap[resortedTalents[j]->index];
                if (!compactToPositionalIndexMap.count(compactIndex)) {
                    compactToPositionalIndexMap[compactIndex] = positionalIndex++;
                }
            }

            std::vector<int> bitToIndexVec;
            bitToIndexVec.reserve(allRunDetails[i].treeDAGInfo->sortedTalents.size());
            for (auto& talent : allRunDetails[i].treeDAGInfo->sortedTalents) {
                bitToIndexVec.push_back(compactToPositionalIndexMap[expandedToCompactIndexMap[talent->index]]);
            }

            allRunDetails[i].bitToIndexVec = bitToIndexVec;

            std::vector<Engine::SIND> switchBits;
            for (size_t bit = 0; bit < allRunDetails[i].treeDAGInfo->sortedTalents.size(); bit++) {
                if (allRunDetails[i].treeDAGInfo->sortedTalents[bit]->type == Engine::TalentType::SWITCH) {
                    switchBits.push_back(bit);
                }
            }

            //if (!settings.solveParallel) {
            if (false) {
                //parallelfor
            }
            else {
                for (auto& comb : allRunDetails[i].treeDAGInfo->allCombinations[0]) {
                    std::vector<int> assignedSwitchIndices;
                    for (auto& bit : switchBits) {
                        bool checkBit = (comb) & (1ULL << bit);
                        if (checkBit) {
                            assignedSwitchIndices.push_back(
                                compactToPositionalIndexMap[expandedToCompactIndexMap[allRunDetails[i].treeDAGInfo->sortedTalents[bit]->index]]
                            );
                        }
                    }
                    allRunDetails[i].assignedSwitchIndices.push_back(assignedSwitchIndices);
                }
            }
        }
    }

    void outputCombinations(std::vector<RunDetails>& allRunDetails, CLSettings& settings) {
        if (!settings.generateOutput) {
            return;
        }
        std::ofstream outFile{ settings.outputFilePath };
        for(RunDetails& details : allRunDetails){
            for (size_t i = 0; i < details.bitToIndexVec.size() - 1; i++) {
                outFile << details.bitToIndexVec[i] << "/";
            }
            outFile << details.bitToIndexVec[details.bitToIndexVec.size() - 1] << "\n";
            for (size_t i = 0; i < details.treeDAGInfo->allCombinations[0].size(); i++) {
                Engine::SIND& comb = details.treeDAGInfo->allCombinations[0][i];
                outFile << comb;
                for (int j = 0; j < static_cast<int>(details.assignedSwitchIndices[i].size()); j++) {
                    outFile << "," << details.assignedSwitchIndices[i][j];
                }
                outFile << "\n";
            }
            outFile << "\n";
        }
    }
}

