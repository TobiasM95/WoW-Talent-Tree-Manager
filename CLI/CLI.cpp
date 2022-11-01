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
            if (component == "--filter-file-path" && argc >= i + 1) {
                settings.filterProvided = true;
                settings.filterFilePath = std::string{ argv[i + 1] };
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
        if (settings.filterProvided) {
            std::cout << "Filter file path:\t" << settings.filterFilePath << "\n";
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
        std::vector<RunDetails> allRunDetails;

        // first parse trees to get tree information and run count
        if (std::filesystem::is_regular_file(settings.structureFilePath)) {
            std::ifstream structureFile(settings.structureFilePath);
            std::string line;

            while (std::getline(structureFile, line)) {
                std::stringstream treeRep{ "" };
                treeRep << line.substr(0, line.find(":"));
                treeRep << ":custom:";
                treeRep << line.substr(line.substr(line.find(":") + 1).find(":") + line.find(":") + 2);

                if (Engine::validateAndRepairTreeStringFormat(treeRep.str())) {
                    RunDetails details;
                    details.tree = Engine::parseTree(treeRep.str());
                    details.targetTalentCount = settings.targetTalentCount;
                    allRunDetails.push_back(details);
                }
            }
        }

        // check if filter provided, if not, update skillsets to empty, if yes, set up filter skillsets
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
    }

    void outputCombinations(std::vector<RunDetails>& allRunDetails, CLSettings& settings) {
        if (!settings.generateOutput) {
            return;
        }
        std::ofstream outFile{ settings.outputFilePath };
        for(RunDetails& details : allRunDetails){
            for (Engine::SIND& comb : details.treeDAGInfo->allCombinations[0]) {
                outFile << comb << "\n";
            }
            outFile << "\n";
        }
    }
}

