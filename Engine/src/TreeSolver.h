#pragma once

#include <vector>
#include <memory>

#include "TTMEnginePresets.h"
#include "TalentTrees.h"

constexpr unsigned long long RESERVED_MEMORY_LIMIT = 4294967296;

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
        std::vector<std::pair<int, int>> switchTalentChoices;
        std::vector<int> rootIndices;
        std::shared_ptr<TalentTree> processedTree;
        vec2d<SIND> allCombinations;
        size_t allCombinationsSum = 0;
        vec2d<SIND> filteredCombinations;
        double elapsedTime = 0.0;
        bool safetyGuardTriggered = false;
        size_t safetyGuard = 500000000;
    };

    struct TreeDAGInfoLegacy {
        vec2d<int> minimalTreeDAG;
        TalentVec sortedTalents;
        std::vector<int> rootIndices;
        std::shared_ptr<TalentTree> processedTree;
        vec2d<std::pair<SIND, int>> allCombinations;
        vec2d<std::pair<SIND, int>> filteredCombinations;
        double elapsedTime = 0.0;
        bool safetyGuardTriggered = false;
    };

    void countConfigurationsFiltered(
        TalentTree tree,
        std::shared_ptr<Engine::TalentSkillset> filter,
        int talentPointsLimit,
        std::shared_ptr<TreeDAGInfo>& treeDAGInfo,
        bool& inProgress,
        bool& safetyGuardTriggered
    );
    void countConfigurationsSingle(
        TalentTree tree,
        int talentPointsLimit,
        std::shared_ptr<TreeDAGInfo>& treeDAGInfo,
        bool& inProgress,
        bool& safetyGuardTriggered
    );
    void countConfigurationsParallel(
        TalentTree tree,
        int talentPointsLimit,
        std::shared_ptr<TreeDAGInfo>& treeDAGInfo,
        bool& inProgress,
        bool& safetyGuardTriggered);
    TreeDAGInfo createSortedMinimalDAG(TalentTree tree);
    TreeDAGInfoLegacy createSortedMinimalDAGLegacy(TalentTree tree);
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
    );
    void visitTalentSingle(
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
        bool& safetyGuardTriggered
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
        vec2d<SIND>& combinations,
        std::vector<int>& allCombinations,
        int& runningCount,
        bool& safetyGuardTriggered
    );
    void visitTalentParallelLegacy(
        std::pair<int, int> talentIndexReqPair,
        SIND visitedTalents,
        int currentPosTalIndex,
        int currentMultiplier,
        int talentPointsSpent,
        int talentPointsLeft,
        std::vector<std::pair<int, int>> possibleTalents,
        const TreeDAGInfoLegacy& sortedTreeDAG,
        vec2d<std::pair<SIND, int>>& combinations,
        std::vector<int>& allCombinations,
        int& runningCount,
        bool& safetyGuardTriggered
    );
    inline void setTalent(SIND& talent, int index);

    std::string fillOutTreeWithBinaryIndexToString(SIND comb, TalentTree tree, TreeDAGInfo treeDAG);
    void insertIntoVector(std::vector<std::pair<int, int>>& v, std::pair<int, int>& t);

    void filterSolvedSkillsets(const TalentTree& tree, std::shared_ptr<TreeDAGInfo> treeDAG, std::shared_ptr<TalentSkillset> filter);
    bool checkSkillsetFilter(
        const SIND visitedTalents,
        const SIND includeFilter,
        const SIND excludeFilter,
        const SIND orFilter,
        const std::vector<std::pair<SIND, SIND>> oneFilter);
    std::shared_ptr<TalentSkillset> skillsetIndexToSkillset(
        const TalentTree& tree,
        std::shared_ptr<TreeDAGInfo> treeDAG,
        SIND skillsetIndex);

    void setSafetyGuard(TreeDAGInfo& treeDAGInfo);
}