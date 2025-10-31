/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Debug/Trace.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/Config.h>
#include <MCore/Source/MultiThreadManager.h>
#include <MCore/Source/RefCounted.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/MotionSet.h>

namespace EMotionFX
{
    // forward declaration
    class Motion;
    class MotionSet;
    class AnimGraph;
    class MotionDataFactory;

    class EMFX_API MotionManager
        : public MCore::RefCounted

    {
        AZ_CLASS_ALLOCATOR_DECL
        friend class Initializer;
        friend class EMotionFXManager;

    public:
        static MotionManager* Create();

        /**
         * Add a motion to the motion manager.
         */
        void AddMotion(Motion* motion);

        /**
         * Get a pointer to the given motion.
         * @param[in] index The index of the motion. The index must be in range [0, GetNumMotions()-1].
         * @return A pointer to the given motion set.
         */
        MCORE_INLINE Motion* GetMotion(size_t index) const                                                      { return m_motions[index]; }

        /**
         * Get the number of motions in the motion manager.
         * @return The number of registered motions.
         */
        MCORE_INLINE size_t GetNumMotions() const                                                               { return m_motions.size(); }

        /**
         * Remove the motion with the given name from the motion manager.
         * @param[in] motionName The name of the motion to remove.
         * @param delFromMemory Set this to true when if you also want to delete the motion from memory.
         *                      When set to false, it will not be deleted from memory, but only removed from the array of motions.
         * @param[in] isTool Set when calling this function from the tools environment (default).
         * @return True in case the motion has been removed successfully. False in case the motion has not been found or the removal failed.
         */
        bool RemoveMotionByName(const char* motionName, bool delFromMemory = true, bool isTool = true);

        /**
         * Remove the motion with the given id from the motion manager.
         * @param[in] id The id of the motion.
         * @param delFromMemory Set this to true when if you also want to delete the motion from memory.
         *                      When set to false, it will not be deleted from memory, but only removed from the array of motions.
         * @return True in case the motion has been removed successfully. False in case the motion has not been found or the removal failed.
         */
        bool RemoveMotionByID(uint32 id, bool delFromMemory = true);

        /**
         * Remove the motion that has a given file name.
         * @param[in] fileName The file name of the motion to delete from the manager.
         * @param delFromMemory Set this to true when if you also want to delete the motion from memory.
         *                      When set to false, it will not be deleted from memory, but only removed from the array of motions.
         * @param[in] isTool Set when calling this function from the tools environment (default).
         * @return True in case the motion has been removed successfully. False in case the motion has not been found or the removal failed.
         */
        bool RemoveMotionByFileName(const char* fileName, bool delFromMemory = true, bool isTool = true);

        /**
         * Remove the given motion from the motion manager.
         * @param[in] motion A pointer to the motion to remove.
         * @param delFromMemory Set this to true when if you also want to delete the motion from memory.
         *                      When set to false, it will not be deleted from memory, but only removed from the array of motions.
         * @return True in case the motion has been removed successfully. False in case the motion has not been found or the removal failed.
         */
        bool RemoveMotion(Motion* motion, bool delFromMemory = true);

        /**
         * Find the given motion by name.
         * @param[in] motionName The name of the motion.
         * @param[in] isTool Set when calling this function from the tools environment (default).
         * @return A pointer to the motion with the given name. nullptr in case the motion has not been found.
         */
        Motion* FindMotionByName(const char* motionName, bool isTool = true) const;

        /**
         * Find the given motion by filename.
         * @param[in] fileName The filename of the motion.
         * @param[in] isTool Set when calling this function from the tools environment (default).
         * @return A pointer to the motion with the given filename. nullptr in case the motion has not been found.
         */
        Motion* FindMotionByFileName(const char* fileName, bool isTool = true) const;

        /**
         * Find the given motion by name.
         * @param[in] id The id of the motion.
         * @return A pointer to the motion with the given id. nullptr in case the motion has not been found.
         */
        Motion* FindMotionByID(uint32 id) const;

        /**
         * Find the motion index by name.
         * @param[in] motionName The name of the motion.
         * @param[in] isTool Set when calling this function from the tools environment (default).
         * @return The index of the motion with the given name. MCORE_INVALIDINDEX32 in case the motion has not been found.
         */
        size_t FindMotionIndexByName(const char* motionName, bool isTool = true) const;

        /**
         * Find the motion index by file name.
         * @param[in] fileName The file name of the motion.
         * @param[in] isTool Set when calling this function from the tools environment (default).
         * @return The index of the motion with the given name. MCORE_INVALIDINDEX32 in case the motion has not been found.
         */
        size_t FindMotionIndexByFileName(const char* fileName, bool isTool = true) const;

        /**
         * Find the motion index by id.
         * @param[in] id The id of the motion.
         * @return The index of the motion with the given id. MCORE_INVALIDINDEX32 in case the motion has not been found.
         */
        size_t FindMotionIndexByID(uint32 id) const;

        /**
         * Find the index for the given motion.
         * @param[in] motion A pointer to the motion to search.
         * @return The index of the motion. MCORE_INVALIDINDEX32 in case the motion has not been found.
         */
        size_t FindMotionIndex(Motion* motion) const;

        /**
         * Add a motion set to the motion manager.
         * @param[in] motionSet A pointer to the motion set to add.
         */
        void AddMotionSet(MotionSet* motionSet);

        /**
         * Get a pointer to the given motion set.
         * @param[in] index The index of the motion set. The index must be in range [0, GetNumMotionSets()-1].
         * @return A pointer to the given motion set.
         */
        MCORE_INLINE MotionSet* GetMotionSet(size_t index) const                                                { return m_motionSets[index]; }

        /**
         * Get the number of motion sets in the motion manager.
         * @return The number of registered motion sets.
         */
        MCORE_INLINE size_t GetNumMotionSets() const                                                            { return m_motionSets.size(); }

        /**
         * Calculate the number of root motion sets.
         * This will iterate over all motion sets, check if they have a parent and sum all the root ones.
         * @return The number of root motion sets.
         */
        size_t CalcNumRootMotionSets() const;

        /**
         * Find the root motion set with the given index.
         * @param[in] index The index of the root motion set. The index must be in range [0, CalcNumRootMotionSets()-1].
         * @return A pointer to the given motion set.
         */
        MotionSet* FindRootMotionSet(size_t index);

        /**
         * Find motion set by name.
         * @param[in] name The name of the motion set.
         * @param[in] isOwnedByRuntime Set when calling this function from Ly environment (default false to mean called from EMotionStudio).
         * @return A pointer to the motion set with the given name. nullptr in case the motion set has not been found.
         */
        MotionSet* FindMotionSetByName(const char* name, bool isOwnedByRuntime = false) const;

        /**
         * Find motion set by id.
         * @param[in] id The id of the motion set.
         * @return A pointer to the motion set with the given id. nullptr in case the motion set has not been found.
         */
        MotionSet* FindMotionSetByID(uint32 id) const;

        /**
         * Find motion set by its filename.
         * @param[in] fileName The filename of the motion set to search for.
         * @param[in] isTool Set when calling this function from the tools environment (default).
         * @return A pointer to the motion set with the given id. nullptr in case the motion set has not been found.
         */
        MotionSet* FindMotionSetByFileName(const char* fileName, bool isTool = true) const;

        /**
         * Find motion set index by name.
         * @param[in] name The name of the motion set.
         * @param[in] isTool Set when calling this function from the tools environment (default).
         * @return The index of the motion set with the given name. MCORE_INVALIDINDEX32 in case the motion has not been found.
         */
        size_t FindMotionSetIndexByName(const char* name, bool isTool = true) const;

        /**
         * Find motion set index by id.
         * @param[in] id The id of the motion set.
         * @return The index of the motion set with the given id. MCORE_INVALIDINDEX32 in case the motion has not been found.
         */
        size_t FindMotionSetIndexByID(uint32 id) const;

        /**
         * Find motion set index for the given motion set.
         * @param[in] motionSet A pointer to the motion set to search.
         * @return The index for the given motion set. MCORE_INVALIDINDEX32 in case the motion has not been found.
         */
        size_t FindMotionSetIndex(MotionSet* motionSet) const;

        bool RemoveMotionSetByName(const char* motionName, bool delFromMemory = true, bool isTool = true);
        bool RemoveMotionSetByID(uint32 id, bool delFromMemory = true);
        bool RemoveMotionSet(MotionSet* motionSet, bool delFromMemory = true);
        void Clear(bool delFromMemory = true);

        void Lock();
        void Unlock();

        MotionDataFactory& GetMotionDataFactory();
        const MotionDataFactory& GetMotionDataFactory() const;

    private:
        AZStd::vector<Motion*>       m_motions;               /**< The array of motions. */
        AZStd::vector<MotionSet*>    m_motionSets;            /**< The array of motion sets. */
        MCore::Mutex                m_lock;                  /**< Motion lock. */
        MCore::Mutex                m_setLock;               /**< The motion set multithread lock. */
        MotionDataFactory*          m_motionDataFactory = nullptr; /**< The motion data factory. */

        //void RecursiveResetMotionNodes(AnimGraphNode* animGraphNode, Motion* motion);
        void ResetMotionNodes(AnimGraph* animGraph, Motion* motion);

        /**
         * Remove the motion at the given index from the motion manager.
         * @param[in] index The index of the motion to remove.
         * @param delFromMemory Set this to true when if you also want to delete the motion from memory.
         *                      When set to false, it will not be deleted from memory, but only removed from the array of motions.
         * @return True in case the motion has been removed successfully. False in case the motion has not been found or the removal failed.
         */
        bool RemoveMotionWithoutLock(size_t index, bool delFromMemory = true);

        bool RemoveMotionSetWithoutLock(size_t index, bool delFromMemory = true);

        MotionManager();
        ~MotionManager() override;
    };
} // namespace EMotionFX
