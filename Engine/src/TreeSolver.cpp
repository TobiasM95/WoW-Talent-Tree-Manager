#include "TreeSolver.h"

#include <iostream>
#include <fstream>
#include <chrono>
#include <algorithm>

namespace Engine {
    void individualCombinationCount() {
        //This snippet runs the configuration count for 1 to 42 available talent points.
        std::ofstream log;
        log.open("log.txt");
        for (int i = 1; i < 20; i++) {
            TalentTree tree = parseTree(
                "0.0:1-+1,2,3;1.0:1-0+4;2.1:2-0+5;3.1:1-0+6;4.0:1-1+10,7;5.0:1-2+;6.0:1-3+8,12,9;7.1:2-4+11;8.1:2-6+11;9.1:2-6+;10.1:3-4+13;11.2:1_0-7,8+14,15;12.1:1-6+16;"
                "13.1:1-10+17,20;14.1:2-11+17;15.1:2-11+18;16.1:1-12+18,19;17.2:1_0-13,14+21;18.1:1-15,16+21;19.1:2-16+22;20.2:1_0-13+23,24,25;21.1:1-17,18+25,26;22.0:1-19+26,27;"
                "23.1:1-20+28;24.1:1-20+;25.1:2-20,21+29;26.1:2-21,22+29;27.1:1-22+30;28.2:1_0-23+;29.2:1_0-25,26+;30.2:1_0-27+;"
            );
            tree.unspentTalentPoints = i;

            auto t1 = std::chrono::high_resolution_clock::now();
            std::unordered_map<std::uint64_t, int> fast_combinations = countConfigurationsFast(tree);
            auto t2 = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> ms_double = t2 - t1;
            log << "Combinations: " << fast_combinations.size() << std::endl;
            log << "Fast operation time: " << ms_double.count() << " ms" << std::endl;
        }
        log.close();

        //Additional useful functions:
        //visualizeTree(tree, "");

        //this function should not be called without knowing what it does, purely for debugging/error checking purposes.
        //but the source code contains more useful function usages to expand/contract trees, converting uint64 indices of DAGs to a tree, etc.
        //compareCombinations(fast_combinations, slow_combinations);
    }

    void parallelCombinationCount() {
        TalentTree tree = parseTree(
            "A1.0:1-+B1,B2,B3;B1.0:1-A1+C1;B2.1:2-A1+C2;B3.1:1-A1+C3;C1.0:1-B1+E1,D1;C2.0:1-B2+;C3.0:1-B3+D2,E4,D3;D1.1:2-C1+E2;D2.1:2-C3+E2;D3.1:2-C3+;E1.1:3-C1+F1;E2.2:1_0-D1,D2+F2,F3;E4.1:1-C3+F4;"
            "F1.1:1-E1+G1,H1;F2.1:2-E2+G1;F3.1:2-E2+G3;F4.1:1-E4+G3,G4;G1.2:1_0-F1,F2+H3;G3.1:1-F3,F4+H3;G4.1:2-F4+H4;H1.2:1_0-F1+I1,I2,I3;H3.1:1-G1,G3+I3,I4;H4.0:1-G4+I4,I5;"
            "I1.1:1-H1+J1;I2.1:1-H1+;I3.1:2-H1,H3+J3;I4.1:2-H3,H4+J3;I5.1:1-H4+J5;J1.2:1_0-I1+;J3.2:1_0-I3,I4+;J5.2:1_0-I5+;"
        );
        tree.unspentTalentPoints = 42;

        auto t1 = std::chrono::high_resolution_clock::now();
        std::vector<std::unordered_map<std::uint64_t, int>> fast_combinations = countConfigurationsFastParallel(tree);
        auto t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> ms_double = t2 - t1;
        std::cout << "Fast parallel operation time: " << ms_double.count() << " ms" << std::endl;
    }

    void testground()
    {
        /*
        std::shared_ptr<Talent> root = createTalent("A1", 1);
        std::shared_ptr<Talent> tb1 = createTalent("B1", 1);
        std::shared_ptr<Talent> tb2 = createTalent("B2", 2);
        std::shared_ptr<Talent> tb3 = createTalent("B3", 1);
        pairTalents(root, tb1);
        pairTalents(root, tb2);
        pairTalents(root, tb3);
        std::cout << printTree(root) << std::endl;
        */

        TalentTree tree = parseTree(
            "A1.0:1-+B1,B2,B3;B1.0:1-A1+C1;B2.1:2-A1+C2;B3.1:1-A1+C3;C1.0:1-B1+E1,D1;C2.0:1-B2+;C3.0:1-B3+D2,E4,D3;D1.1:2-C1+E2;D2.1:2-C3+E2;D3.1:2-C3+;E1.1:3-C1+F1;E2.2:1_0-D1,D2+F2,F3;E4.1:1-C3+F4;"
            "F1.1:1-E1+G1,H1;F2.1:2-E2+G1;F3.1:2-E2+G3;F4.1:1-E4+G3,G4;G1.2:1_0-F1,F2+H3;G3.1:1-F3,F4+H3;G4.1:2-F4+H4;H1.2:1_0-F1+I1,I2,I3;H3.1:1-G1,G3+I3,I4;H4.0:1-G4+I4,I5;"
            "I1.1:1-H1+J1;I2.1:1-H1+;I3.1:2-H1,H3+J3;I4.1:2-H3,H4+J3;I5.1:1-H4+J5;J1.2:1_0-I1+;J3.2:1_0-I3,I4+;J5.2:1_0-I5+;"
        );
        printTree(tree);

        //visualizeTree(tree, "");

        std::cout << "Find all configurations with " << tree.unspentTalentPoints << " available talent points!" << std::endl;

        auto t1 = std::chrono::high_resolution_clock::now();
        std::unordered_map<std::uint64_t, int> fast_combinations = countConfigurationsFast(tree);
        auto t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> ms_double = t2 - t1;
        std::cout << "Fast operation time: " << ms_double.count() << " ms" << std::endl;

        /*
        tree = parseTree(
            "A1.0:1-+B1,B2,B3;B1.0:1-A1+C1;B2.1:2-A1+C2;B3.1:1-A1+C3;C1.0:1-B1+E1,D1;C2.0:1-B2+;C3.0:1-B3+D2,E4,D3;D1.1:2-C1+E2;D2.1:2-C3+E2;D3.1:2-C3+;E1.1:3-C1+F1;E2.2:1_0-D1,D2+F2,F3;E4.1:1-C3+F4;"
            "F1.1:1-E1+G1,H1;F2.1:2-E2+G1;F3.1:2-E2+G3;F4.1:1-E4+G3,G4;G1.2:1_0-F1,F2+H3;G3.1:1-F3,F4+H3;G4.1:2-F4+H4;H1.2:1_0-F1+I1,I2,I3;H3.1:1-G1,G3+I3,I4;H4.0:1-G4+I4,I5;"
            "I1.1:1-H1+J1;I2.1:1-H1+;I3.1:2-H1,H3+J3;I4.1:2-H3,H4+J3;I5.1:1-H4+J5;J1.2:1_0-I1+;J3.2:1_0-I3,I4+;J5.2:1_0-I5+;"
        );
        //expandTreeTalents(tree);
        t1 = std::chrono::high_resolution_clock::now();
        std::unordered_set<std::string> slow_combinations = countConfigurations(tree);
        t2 = std::chrono::high_resolution_clock::now();
        ms_double = t2 - t1;
        std::cout << "Slow operation time: " << ms_double.count() << " ms" << std::endl;
        */

        std::unordered_set<std::string> slow_combinations;
        compareCombinations(fast_combinations, slow_combinations);
    }


    /*
    Counts configurations of a tree with given amount of talent points by topologically sorting the tree and iterating through valid paths (i.e.
    paths with monotonically increasing talent indices). See Wikipedia DAGs (which Wow Talent Trees are) and Topological Sorting.
    Todo: Create a bulk count algorithm that does not employ early stopping if talent tree can't be filled anymore but keeps track of all sub tree binary indices
    to do all different talent points calculations in a single run
    */
    std::unordered_map<std::uint64_t, int> countConfigurationsFast(TalentTree tree) {
        int talentPoints = tree.unspentTalentPoints;
        //expand notes in tree
        expandTreeTalents(tree);
        //visualizeTree(tree, "expanded");

        //create sorted DAG (is vector of vector and at most nx(m+1) Array where n = # nodes and m is the max amount of connections a node has to childs and 
        //+1 because first column contains the weight (1 for regular talents and 2 for switch talents))
        TreeDAGInfo sortedTreeDAG = createSortedMinimalDAG(tree);
        if (sortedTreeDAG.sortedTalents.size() > 64)
            throw std::logic_error("Number of talents exceeds 64, need different indexing type instead of uint64");
        std::unordered_map<std::uint64_t, int> combinations;
        int allCombinations = 0;

        //iterate through all possible combinations in order:
        //have 4 variables: visited nodes (int vector with capacity = # talent points), num talent points left, int vector of possible nodes to visit, weight of combination
        //weight of combination = factor of 2 for every switch talent in path
        std::uint64_t visitedTalents = 0;
        int talentPointsLeft = tree.unspentTalentPoints;
        //note:this will auto sort (not necessary but also doesn't hurt) and prevent duplicates
        std::set<int> possibleTalents;
        //add roots to the list of possible talents first, then iterate recursively with visitTalent
        for (auto& root : sortedTreeDAG.rootIndices) {
            possibleTalents.insert(root);
        }
        for (auto& talent : possibleTalents) {
            //only start with root nodes that have points required == 0, prevents from starting at root nodes that might come later in the tree (e.g. druid wild charge)
            if (sortedTreeDAG.sortedTalents[talent]->pointsRequired == 0)
                visitTalent(talent, visitedTalents, 1, 0, talentPointsLeft, possibleTalents, sortedTreeDAG, combinations, allCombinations);
        }
        std::cout << "Number of configurations for " << talentPoints << " talent points without switch talents: " << combinations.size() << " and with : " << allCombinations << std::endl;

        return combinations;
    }

    /*
    Core recursive function in fast combination counting. Keeps track of selected talents, checks if combination is complete or cannot be finished (early stopping),
    and iterates through possible children in a sorted fashion to prevent duplicates.
    */
    void visitTalent(
        int talentIndex,
        std::uint64_t visitedTalents,
        int currentMultiplier,
        int talentPointsSpent,
        int talentPointsLeft,
        std::set<int> possibleTalents,
        const TreeDAGInfo& sortedTreeDAG,
        std::unordered_map<std::uint64_t, int>& combinations,
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
            combinations[visitedTalents] = currentMultiplier;
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
            possibleTalents.insert(sortedTreeDAG.minimalTreeDAG[talentIndex][i]);
        }
        //visit all possible children while keeping correct order
        for (auto& nextTalent : possibleTalents) {
            //check if next talent is in right order andn talentPointsSpent is >= next talent points required
            if (nextTalent > talentIndex &&
                talentPointsSpent >= sortedTreeDAG.sortedTalents[nextTalent]->pointsRequired) {
                visitTalent(nextTalent, visitedTalents, currentMultiplier, talentPointsSpent, talentPointsLeft, possibleTalents, sortedTreeDAG, combinations, allCombinations);
            }
        }
    }

    /*
    Parallel version of fast configuration counting that runs slower for individual Ns (where N is the amount of available talent points and N >= smallest path from top to bottom)
    compared to single N count but includes all combinations for 1 up to N talent points.
    */
    std::vector<std::unordered_map<std::uint64_t, int>> countConfigurationsFastParallel(TalentTree tree) {
        int talentPoints = tree.unspentTalentPoints;
        //expand notes in tree
        expandTreeTalents(tree);
        //visualizeTree(tree, "expanded");

        //create sorted DAG (is vector of vector and at most nx(m+1) Array where n = # nodes and m is the max amount of connections a node has to childs and 
        //+1 because first column contains the weight (1 for regular talents and 2 for switch talents))
        TreeDAGInfo sortedTreeDAG = createSortedMinimalDAG(tree);
        if (sortedTreeDAG.sortedTalents.size() > 64)
            throw std::logic_error("Number of talents exceeds 64, need different indexing type instead of uint64");
        std::vector<std::unordered_map<std::uint64_t, int>> combinations;
        combinations.resize(talentPoints);
        std::vector<int> allCombinations;
        allCombinations.resize(talentPoints, 0);

        //iterate through all possible combinations in order:
        //have 4 variables: visited nodes (int vector with capacity = # talent points), num talent points left, int vector of possible nodes to visit, weight of combination
        //weight of combination = factor of 2 for every switch talent in path
        std::uint64_t visitedTalents = 0;
        int talentPointsLeft = tree.unspentTalentPoints;
        //note:this will auto sort (not necessary but also doesn't hurt) and prevent duplicates
        std::set<int> possibleTalents;
        //add roots to the list of possible talents first, then iterate recursively with visitTalent
        for (auto& root : sortedTreeDAG.rootIndices) {
            possibleTalents.insert(root);
        }
        for (auto& talent : possibleTalents) {
            //only start with root nodes that have points required == 0, prevents from starting at root nodes that might come later in the tree (e.g. druid wild charge)
            if (sortedTreeDAG.sortedTalents[talent]->pointsRequired == 0)
                visitTalentParallel(talent, visitedTalents, 1, 0, talentPointsLeft, possibleTalents, sortedTreeDAG, combinations, allCombinations);
        }
        for (int i = 0; i < talentPoints; i++) {
            std::cout << "Number of configurations for " << i + 1 << " talent points without switch talents: " << combinations[i].size() << " and with : " << allCombinations[i] << std::endl;
        }

        return combinations;
    }

    /*
    Parallel version of recursive talent visitation that does not early stop and keeps track of all paths shorter than max path length.
    */
    void visitTalentParallel(
        int talentIndex,
        std::uint64_t visitedTalents,
        int currentMultiplier,
        int talentPointsSpent,
        int talentPointsLeft,
        std::set<int> possibleTalents,
        const TreeDAGInfo& sortedTreeDAG,
        std::vector<std::unordered_map<std::uint64_t, int>>& combinations,
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

        combinations[talentPointsSpent - 1][visitedTalents] = currentMultiplier;
        allCombinations[talentPointsSpent - 1] += currentMultiplier;
        if (talentPointsLeft == 0)
            return;

        //add all possible children to the set for iteration
        for (int i = 1; i < sortedTreeDAG.minimalTreeDAG[talentIndex].size(); i++) {
            possibleTalents.insert(sortedTreeDAG.minimalTreeDAG[talentIndex][i]);
        }
        //visit all possible children while keeping correct order
        for (auto& nextTalent : possibleTalents) {
            //check order is correct and if talentPointsSpent is >= next talent points required
            if (nextTalent > talentIndex &&
                talentPointsSpent >= sortedTreeDAG.sortedTalents[nextTalent]->pointsRequired) {
                visitTalentParallel(nextTalent, visitedTalents, currentMultiplier, talentPointsSpent, talentPointsLeft, possibleTalents, sortedTreeDAG, combinations, allCombinations);
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
}