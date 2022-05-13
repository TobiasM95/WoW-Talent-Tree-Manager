#include "TreeSolver.h"

#include <iostream>
#include <fstream>
#include <chrono>
#include <algorithm>

namespace Engine {
    /*
    Counts configurations of a tree with given amount of talent points by topologically sorting the tree and iterating through valid paths (i.e.
    paths with monotonically increasing talent indices). See Wikipedia DAGs (which Wow Talent Trees are) and Topological Sorting.
    Todo: Create a bulk count algorithm that does not employ early stopping if talent tree can't be filled anymore but keeps track of all sub tree binary indices
    to do all different talent points calculations in a single run
    */
    std::shared_ptr<TreeDAGInfo> countConfigurations(TalentTree tree) {
        tree.unspentTalentPoints = tree.maxTalentPoints;
        int talentPoints = tree.unspentTalentPoints;
        //expand notes in tree
        expandTreeTalents(tree);
        //visualizeTree(tree, "expanded");

        //create sorted DAG (is vector of vector and at most nx(m+1) Array where n = # nodes and m is the max amount of connections a node has to childs and 
        //+1 because first column contains the weight (1 for regular talents and 2 for switch talents))
        TreeDAGInfo sortedTreeDAG = createSortedMinimalDAG(tree);
        if (sortedTreeDAG.sortedTalents.size() > 64)
            throw std::logic_error("Number of talents exceeds 64, need different indexing type instead of uint64");
        std::vector<std::pair<std::uint64_t, int>> combinations;
        int allCombinations = 0;

        //iterate through all possible combinations in order:
        //have 4 variables: visited nodes (int vector with capacity = # talent points), num talent points left, int vector of possible nodes to visit, weight of combination
        //weight of combination = factor of 2 for every switch talent in path
        std::uint64_t visitedTalents = 0;
        int talentPointsLeft = tree.unspentTalentPoints;
        //note:this will auto sort (not necessary but also doesn't hurt) and prevent duplicates
        std::vector<int> possibleTalents;
        possibleTalents.reserve(sortedTreeDAG.minimalTreeDAG.size());
        //add roots to the list of possible talents first, then iterate recursively with visitTalent
        for (auto& root : sortedTreeDAG.rootIndices) {
            possibleTalents.push_back(root);
        }
        for (int i = 0; i < possibleTalents.size(); i++) {
            //only start with root nodes that have points required == 0, prevents from starting at root nodes that might come later in the tree (e.g. druid wild charge)
            if (sortedTreeDAG.sortedTalents[possibleTalents[i]]->pointsRequired == 0)
                visitTalent(possibleTalents[i], visitedTalents, i + 1, 1, 0, talentPointsLeft, possibleTalents, sortedTreeDAG, combinations, allCombinations);
        }
        std::cout << "Number of configurations for " << talentPoints << " talent points without switch talents: " << combinations.size() << " and with : " << allCombinations << std::endl;

        std::vector<std::vector<std::pair<std::uint64_t, int>>> allCombinationsVector;
        allCombinationsVector.push_back(combinations);
        sortedTreeDAG.allCombinations = allCombinationsVector;

        return std::make_shared<TreeDAGInfo>(sortedTreeDAG);
    }

    /*
    Core recursive function in fast combination counting. Keeps track of selected talents, checks if combination is complete or cannot be finished (early stopping),
    and iterates through possible children in a sorted fashion to prevent duplicates.
    */
    void visitTalent(
        int talentIndex,
        std::uint64_t visitedTalents,
        int currentPosTalIndex,
        int currentMultiplier,
        int talentPointsSpent,
        int talentPointsLeft,
        std::vector<int> possibleTalents,
        const TreeDAGInfo& sortedTreeDAG,
        std::vector<std::pair<std::uint64_t, int>>& combinations,
        int& allCombinations
    ) {
        /*
        for each node visited add child nodes(in DAG array) to the vector of possible nodesand reduce talent points left
        check if talent points left == 0 (finish) or num_nodes - current_node < talent_points_left (check for off by one error) (cancel cause talent tree can't be filled)
        iterate through all nodes in vector possible nodes to visit but only visit nodes whose index > current index
        if finished perform bit shift on uint64 to get unique tree index and put it in configuration set
        */
        //do combination housekeeping
        setTalent(visitedTalents, talentIndex);
        talentPointsSpent += 1;
        talentPointsLeft -= 1;
        currentMultiplier *= sortedTreeDAG.minimalTreeDAG[talentIndex][0];
        //check if path is complete
        if (talentPointsLeft == 0) {
            combinations.push_back(std::pair<std::uint64_t, int>(visitedTalents, currentMultiplier));
            allCombinations += currentMultiplier;
            return;
        }
        //check if path can be finished (due to sorting and early stopping some paths are ignored even though in practice you could complete them but
        //sorting guarantees that these paths were visited earlier already)
        if (sortedTreeDAG.sortedTalents.size() - talentIndex - 1 < talentPointsLeft) {
            //cannot use up all the leftover talent points, therefore incomplete
            return;
        }
        //add all possible children to the set for iteration
        for (int i = 1; i < sortedTreeDAG.minimalTreeDAG[talentIndex].size(); i++) {
            insertIntoVector(possibleTalents, sortedTreeDAG.minimalTreeDAG[talentIndex][i]);
        }
        //visit all possible children while keeping correct order
        for (int i = currentPosTalIndex; i < possibleTalents.size(); i++) {
            //check if next talent is in right order andn talentPointsSpent is >= next talent points required
            if (possibleTalents[i] > talentIndex &&
                talentPointsSpent >= sortedTreeDAG.sortedTalents[possibleTalents[i]]->pointsRequired) {
                visitTalent(possibleTalents[i], visitedTalents, i + 1, currentMultiplier, talentPointsSpent, talentPointsLeft, possibleTalents, sortedTreeDAG, combinations, allCombinations);
            }
        }
    }

    /*
    Parallel version of fast configuration counting that runs slower for individual Ns (where N is the amount of available talent points and N >= smallest path from top to bottom)
    compared to single N count but includes all combinations for 1 up to N talent points.
    */
    std::shared_ptr<TreeDAGInfo> countConfigurationsParallel(TalentTree tree) {
        std::shared_ptr<TalentTree> processedTree = std::make_shared<TalentTree>(parseTree(createTreeStringRepresentation(tree)));
        tree.unspentTalentPoints = tree.maxTalentPoints;
        int talentPoints = tree.unspentTalentPoints;
        //expand notes in tree
        expandTreeTalents(*processedTree);
        //visualizeTree(tree, "expanded");

        //create sorted DAG (is vector of vector and at most nx(m+1) Array where n = # nodes and m is the max amount of connections a node has to childs and 
        //+1 because first column contains the weight (1 for regular talents and 2 for switch talents))
        TreeDAGInfo sortedTreeDAG = createSortedMinimalDAG(*processedTree);
        sortedTreeDAG.processedTree = processedTree;
        if (sortedTreeDAG.sortedTalents.size() > 64)
            throw std::logic_error("Number of talents exceeds 64, need different indexing type instead of uint64");
        std::vector<std::vector<std::pair<std::uint64_t, int>>> combinations;
        combinations.resize(talentPoints);
        std::vector<int> allCombinations;
        allCombinations.resize(talentPoints, 0);

        //iterate through all possible combinations in order:
        //have 4 variables: visited nodes (int vector with capacity = # talent points), num talent points left, int vector of possible nodes to visit, weight of combination
        //weight of combination = factor of 2 for every switch talent in path
        std::uint64_t visitedTalents = 0;
        int talentPointsLeft = tree.unspentTalentPoints;
        //note:this will auto sort (not necessary but also doesn't hurt) and prevent duplicates
        std::vector<int> possibleTalents;
        //add roots to the list of possible talents first, then iterate recursively with visitTalent
        for (auto& root : sortedTreeDAG.rootIndices) {
            possibleTalents.push_back(root);
        }
        for (int i = 0; i < possibleTalents.size(); i++) {
            //only start with root nodes that have points required == 0, prevents from starting at root nodes that might come later in the tree (e.g. druid wild charge)
            if (sortedTreeDAG.sortedTalents[possibleTalents[i]]->pointsRequired == 0)
                visitTalentParallel(possibleTalents[i], visitedTalents, i + 1, 1, 0, talentPointsLeft, possibleTalents, sortedTreeDAG, combinations, allCombinations);
        }
        for (int i = 0; i < talentPoints; i++) {
            std::cout << "Number of configurations for " << i + 1 << " talent points without switch talents: " << combinations[i].size() << " and with : " << allCombinations[i] << std::endl;
        }
        sortedTreeDAG.allCombinations = combinations;
        return std::make_shared<TreeDAGInfo>(sortedTreeDAG);
    }

    /*
    Parallel version of recursive talent visitation that does not early stop and keeps track of all paths shorter than max path length.
    */
    void visitTalentParallel(
        int talentIndex,
        std::uint64_t visitedTalents,
        int currentPosTalIndex,
        int currentMultiplier,
        int talentPointsSpent,
        int talentPointsLeft,
        std::vector<int> possibleTalents,
        const TreeDAGInfo& sortedTreeDAG,
        std::vector < std::vector < std::pair< std::uint64_t, int>>>& combinations,
        std::vector<int>& allCombinations
    ) {
        /*
        for each node visited add child nodes(in DAG array) to the vector of possible nodesand reduce talent points left
        check if talent points left == 0 (finish) or num_nodes - current_node < talent_points_left (check for off by one error) (cancel cause talent tree can't be filled)
        iterate through all nodes in vector possible nodes to visit but only visit nodes whose index > current index
        if finished perform bit shift on uint64 to get unique tree index and put it in configuration set
        */
        //do combination housekeeping
        setTalent(visitedTalents, talentIndex);
        talentPointsSpent += 1;
        talentPointsLeft -= 1;
        currentMultiplier *= sortedTreeDAG.minimalTreeDAG[talentIndex][0];

        combinations[talentPointsSpent - 1].push_back(std::pair<std::uint64_t, int>(visitedTalents, currentMultiplier));
        allCombinations[talentPointsSpent - 1] += currentMultiplier;
        if (talentPointsLeft == 0)
            return;

        //add all possible children to the set for iteration
        for (int i = 1; i < sortedTreeDAG.minimalTreeDAG[talentIndex].size(); i++) {
            insertIntoVector(possibleTalents, sortedTreeDAG.minimalTreeDAG[talentIndex][i]);
        }
        //visit all possible children while keeping correct order
        for (int i = currentPosTalIndex; i < possibleTalents.size(); i++) {
            //check order is correct and if talentPointsSpent is >= next talent points required
            if (possibleTalents[i] > talentIndex &&
                talentPointsSpent >= sortedTreeDAG.sortedTalents[possibleTalents[i]]->pointsRequired) {
                visitTalentParallel(possibleTalents[i], visitedTalents, i + 1, currentMultiplier, talentPointsSpent, talentPointsLeft, possibleTalents, sortedTreeDAG, combinations, allCombinations);
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
        for (int i = 0; i < tree.talentRoots.size(); i++) {
            info.rootIndices.push_back(i);
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
        std::sort(tree.talentRoots.begin(), tree.talentRoots.end(), [](std::shared_ptr<Talent> a, std::shared_ptr<Talent> b) {
            return a->pointsRequired > b->pointsRequired;
            });
        //while tree.talentRoots is not empty do
        while (tree.talentRoots.size() > 0) {
            //remove a node n from tree.talentRoots
            std::shared_ptr<Talent> n = tree.talentRoots.front();
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
                    std::vector<std::shared_ptr<Talent>>::iterator i = std::find(m->parents.begin(), m->parents.end(), n);
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
                    std::sort(tree.talentRoots.begin(), tree.talentRoots.end(), [](std::shared_ptr<Talent> a, std::shared_ptr<Talent> b) {
                        return a->pointsRequired > b->pointsRequired;
                        });
                }
            }
        }

        //convert sorted talents to minimalTreeDAG representation (raw talents -> integer index vectors)
        for (auto& talent : info.sortedTalents) {
            std::vector<int> child_indices(talent->children.size() + 1);
            child_indices[0] = talent->type == TalentType::SWITCH ? 2 : 1;
            for (int i = 0; i < talent->children.size(); i++) {
                ptrdiff_t pos = std::distance(info.sortedTalents.begin(), std::find(info.sortedTalents.begin(), info.sortedTalents.end(), talent->children[i]));
                if (pos >= info.sortedTalents.size()) {
                    throw std::logic_error("child does not appear in info.sortedTalents");
                }
                child_indices[i + 1] = pos;
            }
            info.minimalTreeDAG.push_back(child_indices);
        }

        return info;
    }

    /*
    Helper function to set a talent as selected (simple bit flip function)
    */
    inline void setTalent(std::uint64_t& talent, int index) {
        talent |= 1ULL << index;
    }

    /*
    Debug function to compare the combinations of the slow legacy method with the fast counting for error checking purposes.
    Creates two files that hold all combinations in the string representation after sorting to make file diff easy and quick.
    */
    void compareCombinations(const std::unordered_map<std::uint64_t, int>& fastCombinations, const std::unordered_set<std::string>& slowCombinations, std::string suffix) {
        std::string directory = "C:\\Users\\Tobi\\Documents\\Programming\\CodeSnippets\\WowTalentTrees\\TreesInputsOutputs";

        std::vector<std::string> fastCombinationsOrdered;
        int count = 0;
        for (auto& comb : fastCombinations) {
            if (count++ % 100 == 0)
                std::cout << count << std::endl;
            TalentTree tree = parseTree(
                "A1.0:1-+B1,B2,B3;B1.0:1-A1+C1;B2.1:2-A1+C2;B3.1:1-A1+C3;C1.0:1-B1+E1,D1;C2.0:1-B2+;C3.0:1-B3+D2,E4,D3;D1.1:2-C1+E2;D2.1:2-C3+E2;D3.1:2-C3+;E1.1:3-C1+F1;E2.2:1_0-D1,D2+F2,F3;E4.1:1-C3+F4;"
                "F1.1:1-E1+G1,H1;F2.1:2-E2+G1;F3.1:2-E2+G3;F4.1:1-E4+G3,G4;G1.2:1_0-F1,F2+H3;G3.1:1-F3,F4+H3;G4.1:2-F4+H4;H1.2:1_0-F1+I1,I2,I3;H3.1:1-G1,G3+I3,I4;H4.0:1-G4+I4,I5;"
                "I1.1:1-H1+J1;I2.1:1-H1+;I3.1:2-H1,H3+J3;I4.1:2-H3,H4+J3;I5.1:1-H4+J5;J1.2:1_0-I1+;J3.2:1_0-I3,I4+;J5.2:1_0-I5+;"
            );
            expandTreeTalents(tree);
            //creating a treeDAG destroys all parents in the tree, but should not be necessary anyway
            TreeDAGInfo treeDAG = createSortedMinimalDAG(tree);
            fastCombinationsOrdered.push_back(fillOutTreeWithBinaryIndexToString(comb.first, tree, treeDAG));
        }
        std::sort(fastCombinationsOrdered.begin(), fastCombinationsOrdered.end());

        std::ofstream output_file_fast(directory + "\\trees_comparison_" + suffix + "_fast.txt");
        std::ostream_iterator<std::string> output_iterator_fast(output_file_fast, "\n");
        std::copy(fastCombinationsOrdered.begin(), fastCombinationsOrdered.end(), output_iterator_fast);

        std::vector<std::string> slowCombinationsOrdered;
        for (auto& comb : slowCombinations) {
            slowCombinationsOrdered.push_back(comb);
        }
        std::sort(slowCombinationsOrdered.begin(), slowCombinationsOrdered.end());

        std::ofstream output_file_slow(directory + "\\trees_comparison_" + suffix + "_slow.txt");
        std::ostream_iterator<std::string> output_iterator_slow(output_file_slow, "\n");
        std::copy(slowCombinationsOrdered.begin(), slowCombinationsOrdered.end(), output_iterator_slow);
    }

    /*
    Recreates a full multi point talents TalentTree based on a uint64 index that holds selected talents. Has the option to filter out trees and visualize them.
    */
    std::string fillOutTreeWithBinaryIndexToString(std::uint64_t comb, TalentTree tree, TreeDAGInfo treeDAG) {
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

    void insertIntoVector(std::vector<int>& v, const int& t) {
        std::vector<int>::iterator i = std::lower_bound(v.begin(), v.end(), t);
        if (i == v.end() || t < *i)
            v.insert(i, t);
    }

    /*
    Filters the skillsets that are created by the tree solver with the given filter
    */
    void filterSolvedSkillsets(TalentTree& tree, std::shared_ptr<TreeDAGInfo> treeDAG, std::shared_ptr<TalentSkillset> filter) {
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
                if (i == 0) {
                    indices.push_back(talent.second->index);
                }
                else {
                    indices.push_back(talent.second->index * tree.maxTalentPoints + (i - 1));
                }
            }
            compactToExpandedIndexMap[talent.second->index] = indices;
        }
        std::uint64_t includeFilter = 0;
        std::uint64_t excludeFilter = 0;
        for (auto& indexFilterPair : filter->assignedSkillPoints) {
            if (indexFilterPair.second == -1) {
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

        std::vector<std::vector<std::pair<std::uint64_t, int>>> filteredCombinations;
        for (int i = 0; i < treeDAG->allCombinations.size(); i++) {
            std::vector<std::pair<std::uint64_t, int>> combs;
            for (int j = 0; j < treeDAG->allCombinations[i].size(); j++) {
                std::uint64_t skillset = treeDAG->allCombinations[i][j].first;
                if ((skillset & excludeFilter) == 0 && ((~skillset) & includeFilter) == 0) {
                    combs.push_back(std::pair<std::uint64_t, int>(skillset, treeDAG->allCombinations[i][j].second));
                }
            }
            filteredCombinations.push_back(combs);
        }

        treeDAG->filteredCombinations = filteredCombinations;
    }

    /*
    Creates a skillset with a given uint64 index
    */
    std::shared_ptr<TalentSkillset> skillsetIndexToSkillset(TalentTree& tree, std::shared_ptr<TreeDAGInfo> treeDAG, std::uint64_t skillsetIndex) {
        std::map<int, int> expandedToCompactIndexMap;
        for (auto& talent : tree.orderedTalents) {
            for (int i = 0; i < talent.second->maxPoints; i++) {
                //this indexing is taken from TalentTrees.cpp expand talent tree routine and represents an arbitrary indexing
                //for multipoint talents that doesn't collide for a use in a map
                if (i == 0) {
                    expandedToCompactIndexMap[talent.second->index] = talent.second->index;
                }
                else {
                    expandedToCompactIndexMap[talent.second->index * tree.maxTalentPoints + (i - 1)] = talent.second->index;
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
                skillset->assignedSkillPoints[compactIndex] += 1;
                skillset->talentPointsSpent += 1;
            }
        }
        return skillset;
    }
}