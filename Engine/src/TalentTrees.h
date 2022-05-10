#pragma once

#include <string>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <algorithm>

namespace Engine {
    // Switch talents can select/switch between 2 talents in the same slot
    enum class TalentType {
        ACTIVE, PASSIVE, SWITCH
    };

    /*
    A talent has an index (scheme: https://github.com/Bloodmallet/simc_support/blob/feature/10-0-experiments/simc_support/game_data/full_tree_coordinates.jpg),
    a name (currently not used), a type, the (max) points and a switch (might make the talent type redundant) as well as a list of all parents and children in
    a simple graph structure.
    */
    struct Talent {
        int index = -1;
        bool isExpanded = false;
        int expansionIndex = 0;
        std::string name = "";
        std::string nameSwitch = "";
        std::vector<std::string> descriptions;
        TalentType type = TalentType::ACTIVE;
        int row = 0;
        int column = 0;
        int points = 0;
        int maxPoints = 1;
        int pointsRequired = 0;
        int talentSwitch = 0;
        std::vector<std::shared_ptr<Talent>> parents;
        std::vector<std::shared_ptr<Talent>> children;

        std::string getName() {
            if (type == TalentType::SWITCH && talentSwitch == 2)
                return nameSwitch;
            else
                return name;
        }

        std::string getDescription() {
            if (type == TalentType::SWITCH)
                return descriptions[talentSwitch < 1 ? 0 : (talentSwitch - 1)];
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
    };

    /*
    A tree has a name, (un)spent talent points and a list of root talents (talents without parents) that are the starting point
    */
    struct TalentTree {
        std::string presetName = "custom";
        std::string name = "defaultTree";
        std::string treeDescription = "";
        std::string loadoutDescription = "";
        int nodeCount = 0;
        int maxTalentPoints = 0;
        int unspentTalentPoints = 30;
        int spentTalentPoints = 0;
        std::vector<std::shared_ptr<Talent>> talentRoots;
        std::map<int, std::shared_ptr<Talent>> orderedTalents;
        std::vector<std::shared_ptr<TalentSkillset>> loadout;
        int activeSkillsetIndex = -1;

        int maxID = 0;
        int maxCol = 0;
        std::map<int, int> talentsPerRow;
    };

    struct TreeCycleCheckFormat {
        std::vector<int> talents;
        std::vector<std::vector<int>> children;
    };

    bool checkIfTreeHasCycle(TalentTree& tree);
    bool checkIfTalentInsertsCycle(TalentTree& tree, std::shared_ptr<Talent> talent);
    bool checkIfParseStringProducesCycle(std::string treeRep);
    TreeCycleCheckFormat createTreeCycleCheckFormat(TalentTree& tree);
    TreeCycleCheckFormat createTreeCycleCheckFormat(TalentTree& tree, std::shared_ptr<Talent> talent);
    TreeCycleCheckFormat createTreeCycleCheckFormat(std::string treeRep);
    bool checkCyclicity(TreeCycleCheckFormat tree);
    bool cycleCheckVisitTalent(int talentIndex, std::vector<int>& talents, std::vector<std::vector<int>> children);

    void updateNodeCountAndMaxTalentPointsAndMaxID(TalentTree& tree);
    void countNodesRecursively(std::unordered_map<int, int>& nodeTalentPoints, int& maxID, int& maxCol, std::unordered_map<int, std::unordered_set<int>>& talentsPerRow, std::shared_ptr<Talent> talent);
    void updateOrderedTalentList(TalentTree& tree);
    void orderTalentsRecursively(std::map<int, std::shared_ptr<Talent>>& talentMap, std::shared_ptr<Talent> child);

    void addChild(std::shared_ptr<Talent> parent, std::shared_ptr<Talent> child);
    void addParent(std::shared_ptr<Talent> child, std::shared_ptr<Talent> parent);
    void pairTalents(std::shared_ptr<Talent> parent, std::shared_ptr<Talent> child);
    std::string getTalentString(TalentTree tree);
    void printTree(TalentTree tree);
    std::string createTreeStringRepresentation(TalentTree tree);
    TalentTree restorePreset(TalentTree& tree, std::string treeRep);
    TalentTree loadTreePreset(std::string treeRep);
    TalentTree parseTree(std::string treeRep);
    TalentTree parseCustomTree(std::string treeRep);
    TalentTree parseTreeFromPreset(std::string treeRep, std::string presetName);
    void addTalentAndChildrenToMap(std::shared_ptr<Talent> talent, std::unordered_map<std::string, int>& treeRepresentation);
    std::string unorderedMapToString(const std::unordered_map<std::string, int>& treeRepresentation, bool sortOutput);
    std::shared_ptr<Talent> createTalent(TalentTree& tree, std::string name, int maxPoints);
    bool validateLoadout(TalentTree& tree, bool addNote);
    bool validateSkillset(TalentTree tree, std::shared_ptr<TalentSkillset> skillset);
    std::vector<std::string> splitString(std::string s, std::string delimiter);
    void visualizeTree(TalentTree root, std::string suffix);
    void visualizeTalentConnections(std::shared_ptr<Talent> root, std::stringstream& connections);
    std::string visualizeTalentInformation(TalentTree tree);
    void getTalentInfos(std::shared_ptr<Talent> talent, std::unordered_map<int, std::string>& talentInfos);
    std::string getFillColor(std::shared_ptr<Talent> talent);
    std::string getShape(TalentType type);
    std::string getSwitchLabel(std::shared_ptr<Talent> talent);

    void autoPositionTreeNodes(TalentTree& tree);
    void findDepthRecursively(int depth, std::shared_ptr<Talent> talent, std::unordered_map<std::shared_ptr<Talent>, int>& maxDepthMap);
    float checkForCrossing(std::shared_ptr<Talent> talent, std::vector<std::vector<int>> edges, std::vector<std::shared_ptr<Talent>> placedTalents);
    float sqDistToParents(std::shared_ptr<Talent> talent);

    void expandTreeTalents(TalentTree& tree);
    void expandTalentAndAdvance(std::shared_ptr<Talent> talent);
    void contractTreeTalents(TalentTree& tree);
    void contractTalentAndAdvance(std::shared_ptr<Talent>& talent);

    void createSkillset(TalentTree& tree);
    void activateSkillset(TalentTree& tree, int index);
    int importSkillsets(TalentTree& tree, std::string importString);
    std::string createSkillsetStringRepresentation(std::shared_ptr<TalentSkillset> skillset);
    std::string createActiveSkillsetStringRepresentation(TalentTree& tree);
    std::string createAllSkillsetsStringRepresentation(TalentTree& tree);

    bool checkTalentValidity(TalentTree& tree);
}