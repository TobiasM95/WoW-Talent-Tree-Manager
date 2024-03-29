#pragma once

#include <string>
#include <locale>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <vector>

#include "TTMEnginePresets.h"

namespace Engine {

    // Switch talents can select/switch between 2 talents in the same slot
    enum class TalentType {
        ACTIVE = 0, 
        PASSIVE = 1, 
        SWITCH = 2
    };

    enum class TreeType {
        CLASS, SPEC
    };
    static const char* TreeTypeDescription[] = { "Class tree", "Spec tree" };

    struct Talent {
        int index = -1;
        int nodeID = -1;
        bool isExpanded = false;
        bool preFilled = false;
        int expansionIndex = 0;
        std::string name = "";
        std::string nameSwitch = "";
        std::vector<std::string> descriptions;
        TalentType type = TalentType::ACTIVE;
        int row = 1;
        int column = 4;
        int points = 0;
        int maxPoints = 1;
        int pointsRequired = 0;
        int talentSwitch = 0;
        std::vector<std::shared_ptr<Talent>> parents;
        std::vector<std::shared_ptr<Talent>> children;
        std::pair<std::string, std::string> iconName = std::pair<std::string, std::string>("default.png", "default.png");

        std::string getName() {
            if (type == TalentType::SWITCH && talentSwitch == 2)
                return nameSwitch;
            else
                return name;
        }

        std::string getNameSwitch() {
            if (type == TalentType::SWITCH && talentSwitch == 2)
                return name;
            else
                return nameSwitch;
        }

        std::string getDescription() {
            if (type == TalentType::SWITCH)
                return descriptions[talentSwitch < 2 ? 0 : 1];
            return descriptions[points < 1 ? 0 : (points - 1)];
        }

        std::string getDescriptionSwitch() {
            if (type == TalentType::SWITCH)
                return descriptions[talentSwitch < 2 ? 1 : 0];
            return descriptions[points < 1 ? 0 : (points - 1)];
        }
    };

    /*
    A talent skillset has a name and a list of integers representing the number of points assigned to a talent in the order that the talent appears in tree.orderedTalents
    */
    struct TalentSkillset {
        std::string name;
        std::map<int, int> assignedSkillPoints;
        int talentPointsSpent = 0;
        int levelCap = 70;
        bool useLevelCap = true;
    };

    /*
    A sim result is a struct that includes a name of the batch (e.g. part of the raidbots url), a vector of skillsets (or talents, whatever you can grab from raidbots), and dps values
    */
    struct SimResult {
        std::string name;
        std::vector<Engine::TalentSkillset> skillsets;
        std::vector<float> dps;
    };

    enum class PerformanceMetric {
        TOP1, TOP3, TOP5, MEDIAN, AVERAGE, TOPMEDIAN
    };

    /*
    Talent performance info holds important information about how the talent performed in the sim result info
    */
    struct TalentPerformanceInfo {
        std::pair<std::pair<std::string, std::string>, float> lowestDPSSkillset;
        std::pair<std::pair<std::string, std::string>, float> medianDPSSkillset;
        std::pair<std::pair<std::string, std::string>, float> averageDPSSkillset;
        std::pair<std::pair<std::string, std::string>, float> highestDPSSkillset;
        std::pair<std::pair<std::string, std::string>, float> lowestDPSSkillsetWithoutTalent;
        std::pair<std::pair<std::string, std::string>, float> medianDPSSkillsetWithoutTalent;
        std::pair<std::pair<std::string, std::string>, float> averageDPSSkillsetWithoutTalent;
        std::pair<std::pair<std::string, std::string>, float> highestDPSSkillsetWithoutTalent;

        std::vector<float> skillsetDPS;
        std::vector<char*> skillsetDPSNames;
    };

    /*
    Analysis result holds the full information about all talents and their performance
    */
    struct AnalysisResult {
        int skillsetCount = 0;
        std::pair<std::pair<std::string, std::string>, float> lowestDPSSkillset;
        std::pair<std::pair<std::string, std::string>, float> medianDPSSkillset;
        std::pair<std::pair<std::string, std::string>, float> averageDPSSkillset;
        std::pair<std::pair<std::string, std::string>, float> highestDPSSkillset;

        //contains the array col position given a talent index, additional ranks are just position + rank
        std::map<int, int> indexToArrayColMap;
        std::vector<std::vector<int>> talentArray;
        std::vector<float> skillsetDPS;
        std::vector<char*> skillsetDPSNames;
        std::vector<TalentPerformanceInfo> talentPerformances;

        //this is needed for color glow texture generation, since that is a separate function
        float minRelPerf = 0.0f;
        std::vector<float> talentAbsolutePositionRankings;
        std::vector<float> talentRelativePerformanceRankings;

        //this is used in the sim analysis window skillset ranking to show relative dps percentage
        float referenceDPS = FLT_MAX;
    };

    /*
    A tree has a name, (un)spent talent points and a list of root talents (talents without parents) that are the starting point
    */
    struct TalentTree {
        std::string presetName = "custom";
        TreeType type = TreeType::CLASS;
        Presets::CLASS_IDS classID = Presets::CLASS_IDS::CLASS_IDS_NONE;
        std::string name = "defaultTree";
        std::string treeDescription = "";
        std::string loadoutDescription = "";
        int nodeCount = 0;
        int maxTalentPoints = 0;
        int preFilledTalentPoints = 0;
        int unspentTalentPoints = 30;
        int spentTalentPoints = 0;
        std::vector<std::shared_ptr<Talent>> talentRoots;
        std::map<int, std::shared_ptr<Talent>> orderedTalents;
        std::vector<std::shared_ptr<TalentSkillset>> loadout;
        int activeSkillsetIndex = -1;

        //complimentary tree for blizzard hash exports
        int complementaryTreeIndex = -1;
        int complementarySkillsetIndex = -1;

        std::vector<std::pair<int, float>> requirementSeparatorInfo;

        int maxID = 0;
        int maxCol = 0;
        std::map<int, int> talentsPerRow;

        int maxRowLimit = 40;
        int maxColumnLimit = 40;

        int selectedSimAnalysisRawResult = -1;
        std::vector<SimResult> simAnalysisRawResults;
        AnalysisResult analysisResult;
    };

    struct TreeCycleCheckFormat {
        std::vector<int> talents;
        vec2d<int> children;
    };

    using Talent_s = std::shared_ptr<Talent>;
    using TalentVec = std::vector<Talent_s>;

    bool checkIfTreeHasCycle(const TalentTree& tree);
    bool checkIfTalentInsertsCycle(const TalentTree& tree, Talent_s talent);
    bool checkIfParseStringProducesCycle(std::string treeRep);
    TreeCycleCheckFormat createTreeCycleCheckFormat(const TalentTree& tree);
    TreeCycleCheckFormat createTreeCycleCheckFormat(const TalentTree& tree, Talent_s talent);
    TreeCycleCheckFormat createTreeCycleCheckFormat(std::string treeRep);
    bool checkCyclicity(TreeCycleCheckFormat tree);
    bool cycleCheckVisitTalent(int talentIndex, std::vector<int>& talents, vec2d<int> children);

    void updateNodeCountAndMaxTalentPointsAndMaxID(TalentTree& tree);
    void countNodesRecursively(std::unordered_map<int, int>& nodeTalentPoints, int& maxID, int& maxCol, std::map<int, int>& preFilledTalentCountMap, std::unordered_map<int, std::unordered_set<int>>& talentsPerRow, Talent_s talent);
    void updateOrderedTalentList(TalentTree& tree);
    void updateRequirementSeparatorInfo(TalentTree& tree);
    void orderTalentsRecursively(std::map<int, Talent_s>& talentMap, Talent_s child);

    void addChild(Talent_s parent, Talent_s child);
    void addParent(Talent_s child, Talent_s parent);
    void pairTalents(Talent_s parent, Talent_s child);
    std::string getTalentString(const TalentTree& tree);
    void printTree(const TalentTree& tree);
    inline std::string cleanString(std::string s);
    inline std::string restoreString(std::string s);
    std::string createTreeStringRepresentation(TalentTree& tree);
    std::string createReadableTreeString(TalentTree& tree);
    Talent_s createTalent(TalentTree& tree, std::string name, int maxPoints);
    bool validateAndRepairTreeStringFormat(std::string treeRep);
    bool validateTalentStringFormat(std::string talentString);
    TalentTree restorePreset(const TalentTree& tree, std::string treeRep);
    TalentTree loadTreePreset(std::string treeRep);
    TalentTree parseTree(std::string treeRep);
    TalentTree parseCustomTree(std::string treeRep);
    TalentTree parseTreeFromPreset(std::string treeRep, std::string presetName);
    void addTalentAndChildrenToMap(Talent_s talent, std::unordered_map<std::string, int>& treeRepresentation);
    std::string unorderedMapToString(const std::unordered_map<std::string, int>& treeRepresentation, bool sortOutput);
    bool validateLoadout(TalentTree& tree, bool addNote);
    bool validateSkillset(TalentTree& tree, std::shared_ptr<TalentSkillset> skillset);
    bool validateSkillsetStringFormat(TalentTree& tree, std::string skillsetString);
    bool validateSkillsetStringFormat(size_t numTalents, std::string skillsetString);
    std::vector<std::string> splitString(std::string s, std::string delimiter);
    std::string extractOrigTalentName(std::string name);
    void visualizeTree(const TalentTree& tree, std::string suffix);
    void visualizeTalentConnections(Talent_s root, std::stringstream& connections);
    std::string visualizeTalentInformation(TalentTree tree);
    void getTalentInfos(Talent_s talent, std::unordered_map<int, std::string>& talentInfos);
    std::string getFillColor(Talent_s talent);
    std::string getShape(TalentType type);
    std::string getSwitchLabel(Talent_s talent);

    void autoPositionTreeNodes(TalentTree& tree);
    bool autoPositionRowNodes(int row, std::map<int, TalentVec>& talentDepths, int maxRow, int maxColumn);
    vec2d<int> createPositionIndices(int row, std::map<int, TalentVec>& talentDepths);
    void expandPosition(int currentIndex, int currentPos, size_t indexSize, size_t additionalPos, std::vector<int> positionVec, vec2d<int>& positions);
    void findDepthRecursively(int depth, Talent_s talent, std::unordered_map<Talent_s, int>& maxDepthMap);
    bool checkForCrossing(int row, std::map<int, TalentVec>& talentDepths);
    bool intersects(std::vector<int> edge1, std::vector<int> edge2);
    void appendEdges(vec2d<int>& edges, int row, std::map<int, TalentVec>& talentDepths);

    void reindexTree(TalentTree& tree);
    void autoPointRequirements(TalentTree& tree);
    void autoShiftTreeToCorner(TalentTree& tree);
    void autoInsertIconNames(std::vector<std::string> iconNames, TalentTree& tree);

    void expandTreeTalents(TalentTree& tree);
    void expandTalentAndAdvance(TalentTree& tree, Talent_s talent, int maxTalentPoints);
    void contractTreeTalents(TalentTree& tree);
    void contractTalentAndAdvance(Talent_s& talent);

    void clearTree(TalentTree& tree);

    void createSkillset(TalentTree& tree);
    void copySkillset(TalentTree& tree, std::shared_ptr<TalentSkillset> skillset);
    void activateSkillset(TalentTree& tree, int index);
    void applyPreselectedTalentsToSkillset(TalentTree& tree, std::shared_ptr<TalentSkillset> skillset);
    std::pair<int, int> importSkillsets(TalentTree& tree, std::string importString);
    std::string createSkillsetStringRepresentation(std::shared_ptr<TalentSkillset> skillset);
    std::string createSkillsetSimcStringRepresentation(std::shared_ptr<TalentSkillset> skillset, const TalentTree& tree);
    std::string createActiveSkillsetStringRepresentation(TalentTree& tree);
    std::string createAllSkillsetsStringRepresentation(TalentTree& tree);
    std::string createSingleTalentsSimcString(TalentTree& tree);
    std::string createSingleTalentComparisonSimcString(TalentTree& tree);
    std::string createActiveSkillsetSimcStringRepresentation(TalentTree& tree, bool createProfileset = false);
    std::string createAllSkillsetsSimcStringRepresentation(TalentTree& tree);
    std::string createAllSkillsetsSimcStringRepresentation(TalentTree& tree, std::vector<std::shared_ptr<TalentSkillset>> loadout);
    size_t get_bit(std::string& hash_string, size_t& head, size_t& byte, const size_t& bits);
    std::pair<std::string, std::string> getClassSpecPresetsFromBlizzHash(std::map<std::string, std::string> presets, std::string& hash_string);
    bool verifyTreeIDWithBlizzHash(const TalentTree& tree, std::string hash_string);
    void exportBlizzardHash(
        const TalentTree& tree,
        const TalentTree* complementaryTree,
        const std::shared_ptr<TalentSkillset> complementarySkillset,
        std::string& hash_string);
    bool importBlizzardHash(
        TalentTree& tree,
        TalentTree* complementaryTree,
        std::string& hash_string,
        bool extractComplementarySkillset
    );
    int getLevelRequirement(const TalentSkillset& sk, const TalentTree& tree, int offset = 0);
    int getLevelRequirement(const int& pointsSpent, const TalentTree& tree, int offset = 0);

    void ImportSimData(std::vector<std::string>& simOutputNames, vec2d<std::string>& simOutputs, TalentTree& tree);
    SimResult ImportSimResult(std::string& simOutputName, std::vector<std::string>& simOutput, TalentTree& tree);
    std::string trim(const std::string& str);

    /*
    Checks if the currently assigned talent points fulfill all the requirements and are valid (in case
    the user edits a skillset and tries to remove a talent point from anywhere in the tree). This is NOT
    a skillset validation routine.
    */
    bool checkTalentValidity(const TalentTree& tree);

    bool repairTreeStringFormat(std::string& treeRep);
    bool repairToV121(std::string& treeRep);
    bool repairToV120(std::string& treeRep);

    std::vector<double> getSimilarityRanking(std::string formattedTalentName, std::vector<std::string> formattedIconNames);
    std::vector<std::string> createWordLetterPairs(std::string name);

    std::string simplifyString(const std::string& s);
    void filterTalentSearch(const std::string& search, Engine::TalentVec& filteredTalents, const TalentTree& tree);

    std::string simcTokenizeName(const std::string& s);
}