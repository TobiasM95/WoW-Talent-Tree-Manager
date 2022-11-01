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

#include "TreeSolver.h"

#include <iostream>
#include <fstream>
#include <chrono>
#include <algorithm>
#include <Windows.h>

namespace Engine {
    /*
    Counts configurations of a tree with given amount of talent points by topologically sorting the tree and iterating through valid paths (i.e.
    paths with monotonically increasing talent indices). See Wikipedia DAGs (which Wow Talent Trees are) and Topological Sorting.
    */
    void countConfigurationsFiltered(
        TalentTree tree,
        std::shared_ptr<Engine::TalentSkillset> filter,
        int talentPointsLimit,
        std::shared_ptr<TreeDAGInfo>& treeDAGInfo,
        bool& inProgress,
        bool& safetyGuardTriggered
    ) {
        inProgress = true;
        std::shared_ptr<TalentTree> processedTree = std::make_shared<TalentTree>(parseTree(createTreeStringRepresentation(tree)));
        tree.unspentTalentPoints = talentPointsLimit;
        int talentPoints = tree.unspentTalentPoints;
        //expand notes in tree
        expandTreeTalents(*processedTree);
        //visualizeTree(tree, "expanded");

        //create sorted DAG (is vector of vector and at most nx(m+1) Array where n = # nodes and m is the max amount of connections a node has to childs and 
        //+1 because first column contains the weight (1 for regular talents and 2 for switch talents))
        TreeDAGInfo sortedTreeDAG = createSortedMinimalDAG(*processedTree);
        setSafetyGuard(sortedTreeDAG);
        sortedTreeDAG.processedTree = processedTree;
        if (sortedTreeDAG.sortedTalents.size() > 64)
            throw std::logic_error("Number of talents exceeds 64, need different indexing type instead of uint64");
        std::vector<SIND> combinations;

        //create filters to be able to filter during solving
        //skillset has compactTalentIndex information
        //treeDAG->sortedTalents containts index->expandedTalentIndex mapping
        //therefore we need compactTalentIndex->expandedTalentIndex mapping and reverse the index->expandedTalentIndex
        std::map<int, int> expandedToPosIndexMap;
        for (int i = 0; i < sortedTreeDAG.sortedTalents.size(); i++) {
            expandedToPosIndexMap[sortedTreeDAG.sortedTalents[i]->index] = i;
        }
        std::map<int, std::vector<int>> compactToExpandedIndexMap;
        for (auto& talent : tree.orderedTalents) {
            std::vector<int> indices;
            for (int i = 0; i < talent.second->maxPoints; i++) {
                //this indexing is taken from TalentTrees.cpp expand talent tree routine and represents an arbitrary indexing
                //for multipoint talents that doesn't collide for a use in a map
                //TTMNOTE: If this changes, also change TalentTrees.cpp->expandTalentAndAdvance and TreeSolver.cpp->skillsetIndexToSkillset
                if (i == 0) {
                    indices.push_back(talent.second->index);
                }
                else {
                    indices.push_back((talent.second->index + 1) * tree.maxTalentPoints + (i - 1));
                }
            }
            compactToExpandedIndexMap[talent.second->index] = indices;
        }
        SIND includeFilter = 0; //this talent has to have exactly the specified amount of talent points
        SIND excludeFilter = 0; //this talent must not have any talent points assigned
        SIND orFilter = 0; //this group of talents has to have at least one talent point assigned
        std::vector<std::pair<SIND, SIND>> oneFilter; //exactly one talent in this group has to be maxed while all others must not have any points
        for (auto& indexFilterPair : filter->assignedSkillPoints) {
            if (indexFilterPair.second == -3) {
                SIND inc = 0;
                SIND exc = 0;
                for (auto& iFP : filter->assignedSkillPoints) {
                    if (iFP.second != -3) {
                        continue;
                    }
                    if (iFP.first == indexFilterPair.first) {
                        for (int i = 0; i < compactToExpandedIndexMap[iFP.first].size(); i++) {
                            int expandedTalentIndex = compactToExpandedIndexMap[iFP.first][i];
                            int pos = expandedToPosIndexMap[expandedTalentIndex];
                            setTalent(inc, pos);
                        }
                    }
                    else {
                        for (int i = 0; i < compactToExpandedIndexMap[iFP.first].size(); i++) {
                            int expandedTalentIndex = compactToExpandedIndexMap[iFP.first][i];
                            int pos = expandedToPosIndexMap[expandedTalentIndex];
                            setTalent(exc, pos);
                        }
                    }
                }
                oneFilter.push_back({ inc, exc });
            }
            if (indexFilterPair.second == -2) {
                for (int i = 0; i < compactToExpandedIndexMap[indexFilterPair.first].size(); i++) {
                    int expandedTalentIndex = compactToExpandedIndexMap[indexFilterPair.first][i];
                    int pos = expandedToPosIndexMap[expandedTalentIndex];
                    setTalent(orFilter, pos);
                }
            }
            else if (indexFilterPair.second == -1) {
                int expandedTalentIndex = compactToExpandedIndexMap[indexFilterPair.first][0];
                int pos = expandedToPosIndexMap[expandedTalentIndex];
                setTalent(excludeFilter, pos);
            }
            else if (indexFilterPair.second > 0) {
                for (int i = 0; i < indexFilterPair.second; i++) {
                    int expandedTalentIndex = compactToExpandedIndexMap[indexFilterPair.first][i];
                    int pos = expandedToPosIndexMap[expandedTalentIndex];
                    setTalent(includeFilter, pos);
                }
            }
        }

        //iterate through all possible combinations in order:
        //have 4 variables: visited nodes (int vector with capacity = # talent points), num talent points left, int vector of possible nodes to visit, weight of combination
        //weight of combination = factor of 2 for every switch talent in path
        SIND visitedTalents = 0;
        int talentPointsLeft = tree.unspentTalentPoints;
        std::vector<std::pair<int, int>> possibleTalents;
        //possibleTalents.reserve(sortedTreeDAG.minimalTreeDAG.size());//is this faster or not?
        //add roots to the list of possible talents first, then iterate recursively with visitTalent
        for (auto& root : sortedTreeDAG.rootIndices) {
            possibleTalents.push_back(std::pair<int, int>(root, sortedTreeDAG.sortedTalents[root]->pointsRequired));
        }
        auto t1 = std::chrono::high_resolution_clock::now();
        //this is used for safeguarding solving process for trees that are too big
        int runningCount = 0;
        for (int i = 0; i < possibleTalents.size(); i++) {
            //only start with root nodes that have points required == 0, prevents from starting at root nodes that might come later in the tree (e.g. druid wild charge)
            if (sortedTreeDAG.sortedTalents[possibleTalents[i].first]->pointsRequired == 0)
                visitTalentFiltered(
                    possibleTalents[i],
                    visitedTalents, 
                    i + 1, 
                    1,
                    0, 
                    talentPointsLeft, 
                    possibleTalents, 
                    sortedTreeDAG, 
                    combinations, 
                    runningCount, 
                    std::ref(safetyGuardTriggered),
                    includeFilter,
                    excludeFilter,
                    orFilter,
                    oneFilter
                );
        }
        if (safetyGuardTriggered) {
            sortedTreeDAG.safetyGuardTriggered = true;
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> ms_double = t2 - t1;
        vec2d<SIND> allCombinationsVector;
        allCombinationsVector.push_back(combinations);
        sortedTreeDAG.allCombinations = allCombinationsVector;
        sortedTreeDAG.elapsedTime = ms_double.count() / 1000.0;
        inProgress = false;

        //collect all switch talent choices
        for (auto& indexTalentPair : tree.orderedTalents) {
            if (indexTalentPair.second->type == Engine::TalentType::SWITCH) {
                sortedTreeDAG.switchTalentChoices.push_back({ indexTalentPair.first, 1 });
            }
        }


        treeDAGInfo = std::make_shared<TreeDAGInfo>(sortedTreeDAG);
    }

    /*
    Core recursive function in fast combination counting. Keeps track of selected talents, checks if combination is complete or cannot be finished (early stopping),
    and iterates through possible children in a sorted fashion to prevent duplicates.
    */
    void visitTalentFiltered(
        std::pair<int, int> talentIndexReqPair,
        SIND visitedTalents,
        int currentPosTalIndex,
        int currentMultiplier,
        int talentPointsSpent,
        int talentPointsLeft,
        std::vector<std::pair<int, int>> possibleTalents,
        const TreeDAGInfo& sortedTreeDAG,
        std::vector<SIND>& combinations,
        int& runningCount,
        bool& safetyGuardTriggered,
        SIND& includeFilter,
        SIND& excludeFilter,
        SIND& orFilter,
        std::vector<std::pair<SIND, SIND>>& oneFilter
    ) {
        /*
        for each node visited add child nodes(in DAG array) to the vector of possible nodesand reduce talent points left
        check if talent points left == 0 (finish) or num_nodes - current_node < talent_points_left (check for off by one error) (cancel cause talent tree can't be filled)
        iterate through all nodes in vector possible nodes to visit but only visit nodes whose index > current index
        if finished perform bit shift on uint64 to get unique tree index and put it in configuration set
        */
        //do combination housekeeping
        setTalent(visitedTalents, talentIndexReqPair.first);
        talentPointsSpent += 1;
        talentPointsLeft -= 1;
        currentMultiplier *= sortedTreeDAG.minimalTreeDAG[talentIndexReqPair.first][0];
        //check if path is complete
        if (talentPointsLeft == 0 && checkSkillsetFilter(visitedTalents, includeFilter, excludeFilter, orFilter, oneFilter)) {
            combinations.push_back(visitedTalents);
            return;
        }
        //check if path can be finished (due to sorting and early stopping some paths are ignored even though in practice you could complete them but
        //sorting guarantees that these paths were visited earlier already)
        //also check if exclude filter is violated and early stop as well
        if (sortedTreeDAG.sortedTalents.size() - talentIndexReqPair.first - 1 < talentPointsLeft
            || (visitedTalents & excludeFilter) != 0) {
            //cannot use up all the leftover talent points, therefore incomplete, or exclude filter was violated
            return;
        }
        //add all possible children to the set for iteration
        for (int i = 1; i < sortedTreeDAG.minimalTreeDAG[talentIndexReqPair.first].size(); i++) {
            int childIndex = sortedTreeDAG.minimalTreeDAG[talentIndexReqPair.first][i];
            std::pair<int, int> targetPair = std::pair<int, int>(childIndex, sortedTreeDAG.sortedTalents[childIndex]->pointsRequired);
            insertIntoVector(possibleTalents, targetPair);
        }
        //visit all possible children while keeping correct order
        for (int i = currentPosTalIndex; i < possibleTalents.size(); i++) {
            //check if next talent is in right order andn talentPointsSpent is >= next talent points required
            if (possibleTalents[i].first > talentIndexReqPair.first &&
                talentPointsSpent >= sortedTreeDAG.sortedTalents[possibleTalents[i].first]->pointsRequired) {
                visitTalentFiltered(
                    possibleTalents[i],
                    visitedTalents, 
                    i + 1, 
                    currentMultiplier, 
                    talentPointsSpent, 
                    talentPointsLeft, 
                    possibleTalents, 
                    sortedTreeDAG, 
                    combinations, 
                    runningCount,
                    std::ref(safetyGuardTriggered),
                    includeFilter,
                    excludeFilter,
                    orFilter,
                    oneFilter);
            }
        }
    }

    /*
    Parallel version of fast configuration counting that runs slower for individual Ns (where N is the amount of available talent points and N >= smallest path from top to bottom)
    compared to single N count but includes all combinations for 1 up to N talent points.
    */
    void countConfigurationsParallel(
        TalentTree tree,
        int talentPointsLimit,
        std::shared_ptr<TreeDAGInfo>& treeDAGInfo,
        bool& inProgress,
        bool& safetyGuardTriggered) {

        inProgress = true;
        std::shared_ptr<TalentTree> processedTree = std::make_shared<TalentTree>(parseTree(createTreeStringRepresentation(tree)));
        tree.unspentTalentPoints = talentPointsLimit;
        int talentPoints = tree.unspentTalentPoints;
        //expand notes in tree
        expandTreeTalents(*processedTree);
        //visualizeTree(tree, "expanded");

        //create sorted DAG (is vector of vector and at most nx(m+1) Array where n = # nodes and m is the max amount of connections a node has to childs and 
        //+1 because first column contains the weight (1 for regular talents and 2 for switch talents))
        TreeDAGInfo sortedTreeDAG = createSortedMinimalDAG(*processedTree);
        setSafetyGuard(sortedTreeDAG);
        sortedTreeDAG.processedTree = processedTree;
        if (sortedTreeDAG.sortedTalents.size() > 64)
            throw std::logic_error("Number of talents exceeds 64, need different indexing type instead of uint64");
        vec2d<SIND> combinations;
        combinations.resize(talentPoints);
        std::vector<int> allCombinations;
        allCombinations.resize(talentPoints, 0);

        //iterate through all possible combinations in order:
        //have 4 variables: visited nodes (int vector with capacity = # talent points), num talent points left, int vector of possible nodes to visit, weight of combination
        //weight of combination = factor of 2 for every switch talent in path
        SIND visitedTalents = 0;
        int talentPointsLeft = tree.unspentTalentPoints;
        std::vector<std::pair<int, int>> possibleTalents;
        //add roots to the list of possible talents first, then iterate recursively with visitTalent
        for (auto& root : sortedTreeDAG.rootIndices) {
            possibleTalents.push_back(std::pair<int, int>(root, sortedTreeDAG.sortedTalents[root]->pointsRequired));
        }
        auto t1 = std::chrono::high_resolution_clock::now();
        //this is used for safeguarding solving process for trees that are too big
        int runningCount = 0;
        for (int i = 0; i < possibleTalents.size(); i++) {
            //only start with root nodes that have points required == 0, prevents from starting at root nodes that might come later in the tree (e.g. druid wild charge)
            //if (possibleTalents[i].second == 0)
            if (sortedTreeDAG.sortedTalents[possibleTalents[i].first]->pointsRequired == 0)
                visitTalentParallel(possibleTalents[i], visitedTalents, i + 1, 1, 0, talentPointsLeft, possibleTalents, sortedTreeDAG, combinations, allCombinations, runningCount, std::ref(safetyGuardTriggered));
        }
        if (safetyGuardTriggered) {
            sortedTreeDAG.safetyGuardTriggered = true;
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> ms_double = t2 - t1;
        sortedTreeDAG.allCombinations = std::move(combinations);
        sortedTreeDAG.elapsedTime = ms_double.count() / 1000.0;
        inProgress = false;

        //collect all switch talent choices
        for (auto& indexTalentPair : tree.orderedTalents) {
            if (indexTalentPair.second->type == Engine::TalentType::SWITCH) {
                sortedTreeDAG.switchTalentChoices.push_back({ indexTalentPair.first, 1 });
            }
        }

        treeDAGInfo = std::make_shared<TreeDAGInfo>(sortedTreeDAG);
    }

    /*
    Parallel version of recursive talent visitation that does not early stop and keeps track of all paths shorter than max path length.
    */
    void visitTalentParallel(
        std::pair<int, int> talentIndexReqPair,
        SIND visitedTalents,
        int currentPosTalIndex,
        int currentMultiplier,
        int talentPointsSpent,
        int talentPointsLeft,
        std::vector<std::pair<int, int>> possibleTalents,
        const TreeDAGInfo& sortedTreeDAG,
        vec2d<SIND>& combinations,
        std::vector<int>& allCombinations,
        int& runningCount,
        bool& safetyGuardTriggered
    ) {
        if (runningCount >= sortedTreeDAG.safetyGuard || safetyGuardTriggered) {
            safetyGuardTriggered = true;
            return;
        }
        /*
        for each node visited add child nodes(in DAG array) to the vector of possible nodesand reduce talent points left
        check if talent points left == 0 (finish) or num_nodes - current_node < talent_points_left (check for off by one error) (cancel cause talent tree can't be filled)
        iterate through all nodes in vector possible nodes to visit but only visit nodes whose index > current index
        if finished perform bit shift on uint64 to get unique tree index and put it in configuration set
        */
        //do combination housekeeping
        setTalent(visitedTalents, talentIndexReqPair.first);
        talentPointsSpent += 1;
        talentPointsLeft -= 1;
        currentMultiplier *= sortedTreeDAG.minimalTreeDAG[talentIndexReqPair.first][0];

        combinations[talentPointsSpent - 1].emplace_back(visitedTalents);
        allCombinations[talentPointsSpent - 1] += currentMultiplier;
        runningCount++;
        if (talentPointsLeft == 0)
            return;

        //add all possible children to the set for iteration
        for (int i = 1; i < sortedTreeDAG.minimalTreeDAG[talentIndexReqPair.first].size(); i++) {
            int childIndex = sortedTreeDAG.minimalTreeDAG[talentIndexReqPair.first][i];
            std::pair<int, int> targetPair = std::pair<int, int>(childIndex, sortedTreeDAG.sortedTalents[childIndex]->pointsRequired);
            insertIntoVector(possibleTalents, targetPair);
        }
        //visit all possible children while keeping correct order
        for (int i = currentPosTalIndex; i < possibleTalents.size(); i++) {
            //check order is correct and if talentPointsSpent is >= next talent points required
            if (possibleTalents[i].first > talentIndexReqPair.first &&
                talentPointsSpent >= sortedTreeDAG.sortedTalents[possibleTalents[i].first]->pointsRequired) {
                visitTalentParallel(possibleTalents[i], visitedTalents, i + 1, currentMultiplier, talentPointsSpent, talentPointsLeft, possibleTalents, sortedTreeDAG, combinations, allCombinations, runningCount, std::ref(safetyGuardTriggered));
            }
        }
    }


    /*
    Parallel version of fast configuration counting that runs slower for individual Ns (where N is the amount of available talent points and N >= smallest path from top to bottom)
    compared to single N count but includes all combinations for 1 up to N talent points.
    */
    std::shared_ptr<TreeDAGInfoLegacy> countConfigurationsParallelLegacy(TalentTree tree, int talentPointsLimit) {
        std::shared_ptr<TalentTree> processedTree = std::make_shared<TalentTree>(parseTree(createTreeStringRepresentation(tree)));
        tree.unspentTalentPoints = talentPointsLimit;
        int talentPoints = tree.unspentTalentPoints;
        //expand notes in tree
        expandTreeTalents(*processedTree);
        //visualizeTree(tree, "expanded");

        //create sorted DAG (is vector of vector and at most nx(m+1) Array where n = # nodes and m is the max amount of connections a node has to childs and 
        //+1 because first column contains the weight (1 for regular talents and 2 for switch talents))
        TreeDAGInfoLegacy sortedTreeDAG = createSortedMinimalDAGLegacy(*processedTree);
        sortedTreeDAG.processedTree = processedTree;
        if (sortedTreeDAG.sortedTalents.size() > 64)
            throw std::logic_error("Number of talents exceeds 64, need different indexing type instead of uint64");
        vec2d<std::pair<SIND, int>> combinations;
        combinations.resize(talentPoints);
        std::vector<int> allCombinations;
        allCombinations.resize(talentPoints, 0);

        //iterate through all possible combinations in order:
        //have 4 variables: visited nodes (int vector with capacity = # talent points), num talent points left, int vector of possible nodes to visit, weight of combination
        //weight of combination = factor of 2 for every switch talent in path
        SIND visitedTalents = 0;
        int talentPointsLeft = tree.unspentTalentPoints;
        //note:this will auto sort (not necessary but also doesn't hurt) and prevent duplicates
        std::vector<std::pair<int, int>> possibleTalents;
        //add roots to the list of possible talents first, then iterate recursively with visitTalent
        for (auto& root : sortedTreeDAG.rootIndices) {
            possibleTalents.push_back(std::pair<int, int>(root, sortedTreeDAG.sortedTalents[root]->pointsRequired));
        }
        auto t1 = std::chrono::high_resolution_clock::now();
        //this is used for safeguarding solving process for trees that are too big
        int runningCount = 0;
        bool safetyGuardTriggered = false;
        for (int i = 0; i < possibleTalents.size(); i++) {
            //only start with root nodes that have points required == 0, prevents from starting at root nodes that might come later in the tree (e.g. druid wild charge)
            //if (possibleTalents[i].second == 0)
            if (sortedTreeDAG.sortedTalents[possibleTalents[i].first]->pointsRequired == 0)
                visitTalentParallelLegacy(possibleTalents[i], visitedTalents, i + 1, 1, 0, talentPointsLeft, possibleTalents, sortedTreeDAG, combinations, allCombinations, runningCount, safetyGuardTriggered);
        }
        if (safetyGuardTriggered) {
            sortedTreeDAG.safetyGuardTriggered = true;
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> ms_double = t2 - t1;
        sortedTreeDAG.allCombinations = std::move(combinations);
        sortedTreeDAG.elapsedTime = ms_double.count() / 1000.0;
        return std::make_shared<TreeDAGInfoLegacy>(sortedTreeDAG);
    }

    /*
    Parallel version of recursive talent visitation that does not early stop and keeps track of all paths shorter than max path length.
    */
    void visitTalentParallelLegacy(
        std::pair<int, int> talentIndexReqPair,
        SIND visitedTalents,
        int currentPosTalIndex,
        int currentMultiplier,
        int talentPointsSpent,
        int talentPointsLeft,
        std::vector<std::pair<int, int>> possibleTalents,
        const TreeDAGInfoLegacy& sortedTreeDAG,
        vec2d<std::pair< SIND, int>>& combinations,
        std::vector<int>& allCombinations,
        int& runningCount,
        bool& safetyGuardTriggered
    ) {
        if (runningCount >= 500000000) {
            safetyGuardTriggered = true;
            return;
        }
        /*
        for each node visited add child nodes(in DAG array) to the vector of possible nodesand reduce talent points left
        check if talent points left == 0 (finish) or num_nodes - current_node < talent_points_left (check for off by one error) (cancel cause talent tree can't be filled)
        iterate through all nodes in vector possible nodes to visit but only visit nodes whose index > current index
        if finished perform bit shift on uint64 to get unique tree index and put it in configuration set
        */
        //do combination housekeeping
        setTalent(visitedTalents, talentIndexReqPair.first);
        talentPointsSpent += 1;
        talentPointsLeft -= 1;
        currentMultiplier *= sortedTreeDAG.minimalTreeDAG[talentIndexReqPair.first][0];

        combinations[talentPointsSpent - 1].emplace_back(visitedTalents, currentMultiplier);
        allCombinations[talentPointsSpent - 1] += currentMultiplier;
        runningCount++;
        if (talentPointsLeft == 0)
            return;

        //add all possible children to the set for iteration
        for (int i = 1; i < sortedTreeDAG.minimalTreeDAG[talentIndexReqPair.first].size(); i++) {
            int childIndex = sortedTreeDAG.minimalTreeDAG[talentIndexReqPair.first][i];
            std::pair<int, int> targetPair = std::pair<int, int>(childIndex, sortedTreeDAG.sortedTalents[childIndex]->pointsRequired);
            insertIntoVector(possibleTalents, targetPair);
        }
        //visit all possible children while keeping correct order
        for (int i = currentPosTalIndex; i < possibleTalents.size(); i++) {
            //check order is correct and if talentPointsSpent is >= next talent points required
            if (possibleTalents[i].first > talentIndexReqPair.first &&
                talentPointsSpent >= sortedTreeDAG.sortedTalents[possibleTalents[i].first]->pointsRequired) {
                visitTalentParallelLegacy(possibleTalents[i], visitedTalents, i + 1, currentMultiplier, talentPointsSpent, talentPointsLeft, possibleTalents, sortedTreeDAG, combinations, allCombinations, runningCount, safetyGuardTriggered);
            }
        }
    }

    /*
    Creates a minimal representation of the TalentTree object as a vector of integer vectors, where each vector has informations about a talent such as switch multiplier
    and child nodes. Wow talent trees are essentially DAGs and can therefore be topologically sorted. TreeDAGInfo contains the minimal tree representation,
    a vector of raw Talents which correspond to the indices in the min. tree rep. and indices of root talents.
    */
    TreeDAGInfo createSortedMinimalDAG(TalentTree tree) {
        //Note: Guarantees that the tree is sorted from left to right first, top to bottom second with each layer of the tree guaranteed to have a lower index than
        //the following layer. This makes it possible to implement min. talent points required for a layer to unlock, checked while iterating in visitTalent, while keeping
        //the algorithm the same and improving speed.
        //note: EVEN THOUGH TREE IS PASSED BY COPY ALL PARENTS WILL BE DELETED FROM TALENTS!
        TreeDAGInfo info;
        std::vector<int> rootIndices;
        for (int i = 0; i < tree.talentRoots.size(); i++) {
            rootIndices.push_back(tree.talentRoots[i]->index);
        }
        //Original Kahn's algorithm description from https://en.wikipedia.org/wiki/Topological_sorting
        /*
        L ← Empty list that will contain the sorted elements    #info.minimalTreeDAG / info.sortedTalents
        S ← Set of all nodes with no incoming edge              #tree.talentRoots

        while S is not empty do
            remove a node n from S
            add n to L
            for each node m with an edge e from n to m do
                remove edge e from the graph
                if m has no other incoming edges then
                    insert m into S

        if graph has edges then
            return error   (graph has at least one cycle)
        else
            return L   (a topologically sorted order)
        */
        std::sort(tree.talentRoots.begin(), tree.talentRoots.end(), [](Talent_s a, Talent_s b) {
            return a->pointsRequired < b->pointsRequired;
            });
        //while tree.talentRoots is not empty do
        while (tree.talentRoots.size() > 0) {
            //remove a node n from tree.talentRoots
            Talent_s n = tree.talentRoots.front();
            //note: terrible in theory but should not matter as vectors are small, otherwise use deque
            tree.talentRoots.erase(tree.talentRoots.begin());
            //add n to info.sortedTalents
            //note: this could be simplified by changing the order of removing from tree.talentRoots and adding n to info.minimalTreeDAG
            info.sortedTalents.push_back(n);
            //for each node m with an edge e from n to m do
            for (auto& m : n->children) {
                //remove edge e from the graph
                //note: it should suffice to just remove the parent in m since every node is only visited once so n->children does not have to be changed inside loop
                if (m->parents.size() > 1) {
                    TalentVec::iterator i = std::find(m->parents.begin(), m->parents.end(), n);
                    if (i != m->parents.end()) {
                        m->parents.erase(i);
                    }
                    else {
                        //If this happens then n has m as child but m does not have n as parent! Bug!
                        throw std::logic_error("child has missing parent");
                    }
                }
                else {
                    m->parents.clear();
                }
                //if m has no other incoming edges then
                if (m->parents.size() == 0) {
                    //insert m into S
                    tree.talentRoots.push_back(m);
                    std::sort(tree.talentRoots.begin(), tree.talentRoots.end(), [](Talent_s a, Talent_s b) {
                        return a->pointsRequired < b->pointsRequired;
                        });
                }
            }
        }
        for (int i = 0; i < info.sortedTalents.size(); i++) {
            for (int j = 0; j < rootIndices.size(); j++) {
                if (info.sortedTalents[i]->index == rootIndices[j]) {
                    info.rootIndices.push_back(i);
                    break;
                }
            }
        }

        //convert sorted talents to minimalTreeDAG representation (raw talents -> integer index vectors)
        for (auto& talent : info.sortedTalents) {
            std::vector<int> child_indices(talent->children.size() + 1);
            child_indices[0] = talent->type == TalentType::SWITCH ? 2 : 1;
            for (int i = 0; i < talent->children.size(); i++) {
                size_t pos = static_cast<size_t>(std::distance(info.sortedTalents.begin(), std::find(info.sortedTalents.begin(), info.sortedTalents.end(), talent->children[i])));
                if (pos >= info.sortedTalents.size()) {
                    throw std::logic_error("child does not appear in info.sortedTalents");
                }
                child_indices[i + 1] = static_cast<int>(pos);
            }
            info.minimalTreeDAG.push_back(child_indices);
        }

        return info;
    }

    TreeDAGInfoLegacy createSortedMinimalDAGLegacy(TalentTree tree) {
        //Note: Guarantees that the tree is sorted from left to right first, top to bottom second with each layer of the tree guaranteed to have a lower index than
        //the following layer. This makes it possible to implement min. talent points required for a layer to unlock, checked while iterating in visitTalent, while keeping
        //the algorithm the same and improving speed.
        //note: EVEN THOUGH TREE IS PASSED BY COPY ALL PARENTS WILL BE DELETED FROM TALENTS!
        TreeDAGInfoLegacy info;
        std::vector<int> rootIndices;
        for (int i = 0; i < tree.talentRoots.size(); i++) {
            rootIndices.push_back(tree.talentRoots[i]->index);
        }
        //Original Kahn's algorithm description from https://en.wikipedia.org/wiki/Topological_sorting
        /*
        L ← Empty list that will contain the sorted elements    #info.minimalTreeDAG / info.sortedTalents
        S ← Set of all nodes with no incoming edge              #tree.talentRoots

        while S is not empty do
            remove a node n from S
            add n to L
            for each node m with an edge e from n to m do
                remove edge e from the graph
                if m has no other incoming edges then
                    insert m into S

        if graph has edges then
            return error   (graph has at least one cycle)
        else
            return L   (a topologically sorted order)
        */
        std::sort(tree.talentRoots.begin(), tree.talentRoots.end(), [](Talent_s a, Talent_s b) {
            return a->pointsRequired < b->pointsRequired;
            });
        //while tree.talentRoots is not empty do
        while (tree.talentRoots.size() > 0) {
            //remove a node n from tree.talentRoots
            Talent_s n = tree.talentRoots.front();
            //note: terrible in theory but should not matter as vectors are small, otherwise use deque
            tree.talentRoots.erase(tree.talentRoots.begin());
            //add n to info.sortedTalents
            //note: this could be simplified by changing the order of removing from tree.talentRoots and adding n to info.minimalTreeDAG
            info.sortedTalents.push_back(n);
            //for each node m with an edge e from n to m do
            for (auto& m : n->children) {
                //remove edge e from the graph
                //note: it should suffice to just remove the parent in m since every node is only visited once so n->children does not have to be changed inside loop
                if (m->parents.size() > 1) {
                    TalentVec::iterator i = std::find(m->parents.begin(), m->parents.end(), n);
                    if (i != m->parents.end()) {
                        m->parents.erase(i);
                    }
                    else {
                        //If this happens then n has m as child but m does not have n as parent! Bug!
                        throw std::logic_error("child has missing parent");
                    }
                }
                else {
                    m->parents.clear();
                }
                //if m has no other incoming edges then
                if (m->parents.size() == 0) {
                    //insert m into S
                    tree.talentRoots.push_back(m);
                    std::sort(tree.talentRoots.begin(), tree.talentRoots.end(), [](Talent_s a, Talent_s b) {
                        return a->pointsRequired < b->pointsRequired;
                        });
                }
            }
        }
        for (int i = 0; i < info.sortedTalents.size(); i++) {
            for (int j = 0; j < rootIndices.size(); j++) {
                if (info.sortedTalents[i]->index == rootIndices[j]) {
                    info.rootIndices.push_back(i);
                    break;
                }
            }
        }

        //convert sorted talents to minimalTreeDAG representation (raw talents -> integer index vectors)
        for (auto& talent : info.sortedTalents) {
            std::vector<int> child_indices(talent->children.size() + 1);
            child_indices[0] = talent->type == TalentType::SWITCH ? 2 : 1;
            for (int i = 0; i < talent->children.size(); i++) {
                size_t pos = static_cast<size_t>(std::distance(info.sortedTalents.begin(), std::find(info.sortedTalents.begin(), info.sortedTalents.end(), talent->children[i])));
                if (pos >= info.sortedTalents.size()) {
                    throw std::logic_error("child does not appear in info.sortedTalents");
                }
                child_indices[i + 1] = static_cast<int>(pos);
            }
            info.minimalTreeDAG.push_back(child_indices);
        }

        return info;
    }

    /*
    Helper function to set a talent as selected (simple bit flip function)
    */
    inline void setTalent(SIND& talent, int index) {
        talent |= 1ULL << index;
    }

    /*
    Recreates a full multi point talents TalentTree based on a uint64 index that holds selected talents. Has the option to filter out trees and visualize them.
    */
    std::string fillOutTreeWithBinaryIndexToString(SIND comb, TalentTree tree, TreeDAGInfo treeDAG) {
        bool filterFlag = false;
        for (int i = 0; i < 64; i++) {
            //check if bit is on
            bool bitSet = (comb >> i) & 1U;
            //select talent in tree
            if (bitSet) {
                if (i >= treeDAG.sortedTalents.size())
                    throw std::logic_error("bit of a talent that does not exist is set!");
                treeDAG.sortedTalents[i]->points = 1;
                if (i == 190000)
                    filterFlag = true;
            }
        }
        contractTreeTalents(tree);
        if (filterFlag)
            visualizeTree(tree, "7points_E2_" + std::to_string(comb));

        return getTalentString(tree);
    }

    void insertIntoVector(std::vector<std::pair<int, int>>& v, std::pair<int, int>& t) {
        std::vector<std::pair<int, int>>::iterator i = std::lower_bound(v.begin(), v.end(), t);
        if (i == v.end() || t < *i)
            v.insert(i, t);
    }

    /*
    Filters the skillsets that are created by the tree solver with the given filter
    */
    void filterSolvedSkillsets(const TalentTree& tree, std::shared_ptr<TreeDAGInfo> treeDAG, std::shared_ptr<TalentSkillset> filter) {
        treeDAG->filteredCombinations.clear();
        //skillset has compactTalentIndex information
        //treeDAG->sortedTalents containts index->expandedTalentIndex mapping
        //therefore we need compactTalentIndex->expandedTalentIndex mapping and reverse the index->expandedTalentIndex
        std::map<int, int> expandedToPosIndexMap;
        for (int i = 0; i < treeDAG->sortedTalents.size(); i++) {
            expandedToPosIndexMap[treeDAG->sortedTalents[i]->index] = i;
        }
        std::map<int, std::vector<int>> compactToExpandedIndexMap;
        for (auto& talent : tree.orderedTalents) {
            std::vector<int> indices;
            for (int i = 0; i < talent.second->maxPoints; i++) {
                //this indexing is taken from TalentTrees.cpp expand talent tree routine and represents an arbitrary indexing
                //for multipoint talents that doesn't collide for a use in a map
                //TTMNOTE: If this changes, also change TalentTrees.cpp->expandTalentAndAdvance and TreeSolver.cpp->skillsetIndexToSkillset
                if (i == 0) {
                    indices.push_back(talent.second->index);
                }
                else {
                    indices.push_back((talent.second->index + 1) * tree.maxTalentPoints + (i - 1));
                }
            }
            compactToExpandedIndexMap[talent.second->index] = indices;
        }
        SIND includeFilter = 0; //this talent has to have exactly the specified amount of talent points
        SIND excludeFilter = 0; //this talent must not have any talent points assigned
        SIND orFilter = 0; //this group of talents has to have at least one talent point assigned
        std::vector<std::pair<SIND, SIND>> oneFilter; //exactly one talent in this group has to be maxed while all others must not have any points
        for (auto& indexFilterPair : filter->assignedSkillPoints) {
            if (indexFilterPair.second == -3) {
                SIND inc = 0;
                SIND exc = 0;
                for (auto& iFP : filter->assignedSkillPoints) {
                    if (iFP.second != -3) {
                        continue;
                    }
                    if (iFP.first == indexFilterPair.first) {
                        for (int i = 0; i < compactToExpandedIndexMap[iFP.first].size(); i++) {
                            int expandedTalentIndex = compactToExpandedIndexMap[iFP.first][i];
                            int pos = expandedToPosIndexMap[expandedTalentIndex];
                            setTalent(inc, pos);
                        }
                    }
                    else {
                        for (int i = 0; i < compactToExpandedIndexMap[iFP.first].size(); i++) {
                            int expandedTalentIndex = compactToExpandedIndexMap[iFP.first][i];
                            int pos = expandedToPosIndexMap[expandedTalentIndex];
                            setTalent(exc, pos);
                        }
                    }
                }
                oneFilter.push_back({ inc, exc });
            }
            if (indexFilterPair.second == -2) {
                for (int i = 0; i < compactToExpandedIndexMap[indexFilterPair.first].size(); i++) {
                    int expandedTalentIndex = compactToExpandedIndexMap[indexFilterPair.first][i];
                    int pos = expandedToPosIndexMap[expandedTalentIndex];
                    setTalent(orFilter, pos);
                }
            }
            else if (indexFilterPair.second == -1) {
                int expandedTalentIndex = compactToExpandedIndexMap[indexFilterPair.first][0];
                int pos = expandedToPosIndexMap[expandedTalentIndex];
                setTalent(excludeFilter, pos);
            }
            else if (indexFilterPair.second > 0) {
                for (int i = 0; i < indexFilterPair.second; i++) {
                    int expandedTalentIndex = compactToExpandedIndexMap[indexFilterPair.first][i];
                    int pos = expandedToPosIndexMap[expandedTalentIndex];
                    setTalent(includeFilter, pos);
                }
            }
        }

        vec2d<SIND> filteredCombinations;
        if (includeFilter == 0 && excludeFilter == 0 && orFilter == 0 && oneFilter.size() == 0) {
            treeDAG->filteredCombinations = treeDAG->allCombinations;
            return;
        }
        for (int i = 0; i < treeDAG->allCombinations.size(); i++) {
            std::vector<SIND> combs;
            combs.reserve(treeDAG->allCombinations[i].size());
            for (int j = 0; j < treeDAG->allCombinations[i].size(); j++) {
                SIND skillset = treeDAG->allCombinations[i][j];
                if ((skillset & excludeFilter) == 0 && ((~skillset) & includeFilter) == 0 && (orFilter == 0 || (skillset & orFilter) > 0)) {
                    if (oneFilter.size() == 0) {
                        combs.push_back(skillset);
                    }
                    else {
                        size_t matches = 0;
                        for (auto& filterPair : oneFilter) {
                            if (((~skillset) & filterPair.first) == 0 && (skillset & filterPair.second) == 0) {
                                matches += 1;
                            }
                        }
                        if (matches == 1) {
                            combs.push_back(skillset);
                        }
                    }
                }
            }
            filteredCombinations.push_back(combs);
        }

        treeDAG->filteredCombinations = std::move(filteredCombinations);
    }

    inline bool checkSkillsetFilter(
        const SIND skillset, 
        const SIND includeFilter,
        const SIND excludeFilter,
        const SIND orFilter,
        const std::vector<std::pair<SIND, SIND>> oneFilter) {
        if (includeFilter == 0 && excludeFilter == 0 && orFilter == 0 && oneFilter.size() == 0) {
            return true;
        }
        if ((skillset & excludeFilter) == 0 && ((~skillset) & includeFilter) == 0 && (orFilter == 0 || (skillset & orFilter) > 0)) {
            if (oneFilter.size() == 0) {
                return true;
            }
            else {
                size_t matches = 0;
                for (auto& filterPair : oneFilter) {
                    if (((~skillset) & filterPair.first) == 0 && (skillset & filterPair.second) == 0) {
                        matches += 1;
                    }
                }
                if (matches == 1) {
                    return true;
                }
            }
        }
        return false;
    }

    /*
    Creates a skillset with a given uint64 index
    */
    std::shared_ptr<TalentSkillset> skillsetIndexToSkillset(
        const TalentTree& tree, 
        std::shared_ptr<TreeDAGInfo> treeDAG, 
        SIND skillsetIndex) 
    {
        std::map<int, int> expandedToCompactIndexMap;
        for (auto& talent : tree.orderedTalents) {
            for (int i = 0; i < talent.second->maxPoints; i++) {
                //this indexing is taken from TalentTrees.cpp expand talent tree routine and represents an arbitrary indexing
                //for multipoint talents that doesn't collide for a use in a map
                //TTMNOTE: If this changes, also change TalentTrees.cpp->expandTalentAndAdvance and TreeSolver.cpp->filterSolvedSkillsets
                if (i == 0) {
                    expandedToCompactIndexMap[talent.second->index] = talent.second->index;
                }
                else {
                    expandedToCompactIndexMap[(talent.second->index + 1) * tree.maxTalentPoints + (i - 1)] = talent.second->index;
                }
            }
        }
        std::shared_ptr<TalentSkillset> skillset = std::make_shared<TalentSkillset>();
        skillset->name = "New skillset";
        for (auto& talent : tree.orderedTalents) {
            skillset->assignedSkillPoints[talent.first] = 0;
        }
        for (int i = 0; i < treeDAG->sortedTalents.size(); i++) {
            bool checkBit = (skillsetIndex) & (1ULL << i);
            if (checkBit) {
                int compactIndex = expandedToCompactIndexMap[treeDAG->sortedTalents[i]->index];
                skillset->talentPointsSpent += 1;
                if (treeDAG->sortedTalents[i]->type == TalentType::SWITCH) {
                    skillset->assignedSkillPoints[compactIndex] = 1;
                    for (auto& indexChoicePair : treeDAG->switchTalentChoices) {
                        if (indexChoicePair.first == compactIndex) {
                            skillset->assignedSkillPoints[compactIndex] = indexChoicePair.second;
                        }
                    }
                }
                else {
                    skillset->assignedSkillPoints[compactIndex] += 1;
                }
            }
        }
        return skillset;
    }

    void setSafetyGuard(TreeDAGInfo& treeDAGInfo) {
        MEMORYSTATUSEX status;
        status.dwLength = sizeof(status);
        GlobalMemoryStatusEx(&status);
        treeDAGInfo.safetyGuard = static_cast<size_t>((status.ullTotalPhys - RESERVED_MEMORY_LIMIT) * 0.5 * 0.125);
    }
}