#pragma once

#include <vector>
#include <memory>

#include "TTMEnginePresets.h"
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
        vec2d<int> minimalTreeDAG;
        TalentVec sortedTalents;
        std::vector<int> rootIndices;
        std::shared_ptr<TalentTree> processedTree;
        vec2d<std::pair<SIND, int>> allCombinations;
        vec2d<std::pair<SIND, int>> filteredCombinations;
        double elapsedTime;
    }; 

    std::shared_ptr<TreeDAGInfo> countConfigurations(TalentTree tree, int talentPointsLimit);
    std::shared_ptr<TreeDAGInfo> countConfigurationsParallel(TalentTree tree, int talentPointsLimit);
    TreeDAGInfo createSortedMinimalDAG(TalentTree tree);
    void visitTalent(
        std::pair<int, int> talentIndexReqPair,
        SIND visitedTalents,
        int currentPosTalIndex,
        int currentMultiplier,
        int talentPointsSpent,
        int talentPointsLeft,
        std::vector<std::pair<int, int>> possibleTalents,
        const TreeDAGInfo& sortedTreeDAG,
        std::vector<std::pair<SIND, int>>& combinations,
        int& allCombinations
    );
    void visitTalentParallel(
        std::pair<int, int> talentIndexReqPair,
        SIND visitedTalents,
        int currentPosTalIndex,
        int currentMultiplier,
        int talentPointsSpent,
        int talentPointsLeft,
        std::vector<std::pair<int, int>> possibleTalents,
        const TreeDAGInfo& sortedTreeDAG,
        vec2d<std::pair< SIND, int>>& combinations,
        std::vector<int>& allCombinations
    );
    inline void setTalent(SIND& talent, int index);

    std::string fillOutTreeWithBinaryIndexToString(SIND comb, TalentTree tree, TreeDAGInfo treeDAG);
    void insertIntoVector(std::vector<std::pair<int, int>>& v, std::pair<int, int>& t);

    void filterSolvedSkillsets(const TalentTree& tree, std::shared_ptr<TreeDAGInfo> treeDAG, std::shared_ptr<TalentSkillset> filter);
    std::shared_ptr<TalentSkillset> skillsetIndexToSkillset(const TalentTree& tree, std::shared_ptr<TreeDAGInfo> treeDAG, SIND);
}