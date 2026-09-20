#pragma once

#include <vector>
#include <memory>
#include <chrono>

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

        /*
        Count without storing. Enumeration cost is unchanged but memory becomes O(1)
        instead of O(results), which is the difference between answering "how many
        valid builds exist?" and materialising them.

        Measured: an exhaustive 30 point solve of a real spec tree yields ~305 million
        combinations and a 9.5 GB result. Counting needs only the counter.
        When set, resultCount is still filled but allCombinations stays empty.
        */
        bool countOnly = false;
        size_t resultCount = 0;

        /*
        Caller-chosen cap on stored/counted results. 0 means "not set", in which case
        setSafetyGuard derives one from available memory.

        This is deliberately separate from safetyGuard: that field is default-
        constructed to 500,000,000, so it cannot distinguish "the caller wants this
        limit" from "nobody touched it", and treating it as an override silently
        clamped every solve to the struct default.
        */
        size_t safetyGuardOverride = 0;

        /*
        Wall-clock budget in milliseconds; 0 means unlimited.

        The engine had a combination-count guard and a memory guard but no time guard at
        all, so nothing stopped a solve from running arbitrarily long. On a shared server
        that is the difference between a slow request and a wedged worker: five talent
        points separate a 0.12 s solve from a 452 s one.
        */
        size_t timeBudgetMs = 0;
        bool timedOut = false;
    };

    /*
    Tracks a solve's wall-clock deadline. Checking the clock at every node would cost more
    than the node's work, so the check is sampled -- accurate to a few milliseconds, which
    is all a budget in seconds needs.
    */
    struct SolveDeadline {
        std::chrono::steady_clock::time_point deadline{};
        bool unlimited = true;
        unsigned int tick = 0;
        bool expired = false;

        static constexpr unsigned int CHECK_INTERVAL = 8192;

        bool exceeded() {
            if (unlimited || expired) {
                return expired;
            }
            if ((++tick % CHECK_INTERVAL) != 0) {
                return false;
            }
            if (std::chrono::steady_clock::now() >= deadline) {
                expired = true;
            }
            return expired;
        }
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
        size_t& runningCount,
        bool& safetyGuardTriggered,
        SIND& includeFilter,
        SIND& excludeFilter,
        SIND& orFilter,
        std::vector<std::pair<SIND, SIND>>& oneFilter,
        SolveDeadline& deadline
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
        size_t& runningCount,
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
        std::vector<size_t>& allCombinations,
        size_t& runningCount,
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
        std::vector<size_t>& allCombinations,
        size_t& runningCount,
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