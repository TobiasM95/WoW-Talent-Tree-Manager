#pragma once

#include <vector>
#include <memory>

#include "TalentTrees.h"

namespace Engine {
    /*
    This is the container for the heavily optimized, topologically sorted DAG variant of the talent tree.
    The regular talent tree has all the meta information and easy readable/debugable structures whereas this container
    only has integer indices with an unconnected raw list of talents for computational efficieny.
    NOTE: The talents aren't selected (i.e. Talent::points incremented) at all but a flag is set in a uint64 which is used
    as an indexer. There exist routines that translate from uint64 to a regular tree and in the future maybe vice versa.
    */
    struct TreeDAGInfo {
        std::vector<std::vector<int>> minimalTreeDAG;
        std::vector<std::shared_ptr<Talent>> sortedTalents;
        std::vector<int> rootIndices;
        std::shared_ptr<TalentTree> processedTree;
        std::vector<std::vector<std::pair<std::uint64_t, int>>> allCombinations;
    }; 

    std::shared_ptr<TreeDAGInfo> countConfigurations(TalentTree tree);
    std::shared_ptr<TreeDAGInfo> countConfigurationsParallel(TalentTree tree);
    TreeDAGInfo createSortedMinimalDAG(TalentTree tree);
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
    );
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
    );
    inline void setTalent(std::uint64_t& talent, int index);

    std::string fillOutTreeWithBinaryIndexToString(std::uint64_t comb, TalentTree tree, TreeDAGInfo treeDAG);
    void insertIntoVector(std::vector<int>& v, const int& t);
}