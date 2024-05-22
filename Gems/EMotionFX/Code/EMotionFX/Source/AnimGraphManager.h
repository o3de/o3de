/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "EMotionFXConfig.h"
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/RefCounted.h>
#include <AzCore/std/containers/vector.h>
#include "AnimGraphObject.h"
#include <MCore/Source/MultiThreadManager.h>


namespace EMotionFX
{
    // forward declarations
    class BlendSpaceManager;
    class AnimGraph;
    class AnimGraphObjectFactory;
    class AnimGraphInstance;
    class ActorInstance;


    /**
     *
     *
     */
    class EMFX_API AnimGraphManager
        : public MCore::RefCounted
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        static AnimGraphManager* Create();

        void Init();

        MCORE_INLINE BlendSpaceManager* GetBlendSpaceManager() const { return m_blendSpaceManager; }

        // anim graph helper functions
        void AddAnimGraph(AnimGraph* setup);
        void RemoveAnimGraph(size_t index, bool delFromMemory = true);
        bool RemoveAnimGraph(AnimGraph* animGraph, bool delFromMemory = true);
        void RemoveAllAnimGraphs(bool delFromMemory = true);

        MCORE_INLINE size_t GetNumAnimGraphs() const                                { MCore::LockGuardRecursive lock(m_animGraphLock); return m_animGraphs.size(); }
        MCORE_INLINE AnimGraph* GetAnimGraph(size_t index) const                    { MCore::LockGuardRecursive lock(m_animGraphLock); return m_animGraphs[index]; }
        AnimGraph* GetFirstAnimGraph() const;

        size_t FindAnimGraphIndex(AnimGraph* animGraph) const;
        AnimGraph* FindAnimGraphByFileName(const char* filename, bool isTool = true) const;
        AnimGraph* FindAnimGraphByID(uint32 animGraphID) const;

        // anim graph instance helper functions
        void AddAnimGraphInstance(AnimGraphInstance* animGraphInstance);
        void RemoveAnimGraphInstance(size_t index, bool delFromMemory = true);
        bool RemoveAnimGraphInstance(AnimGraphInstance* animGraphInstance, bool delFromMemory = true);
        void RemoveAnimGraphInstances(AnimGraph* animGraph, bool delFromMemory = true);
        void RemoveAllAnimGraphInstances(bool delFromMemory = true);
        void InvalidateInstanceUniqueDataUsingMotionSet(EMotionFX::MotionSet* motionSet);

        size_t GetNumAnimGraphInstances() const                        { MCore::LockGuardRecursive lock(m_animGraphInstanceLock); return m_animGraphInstances.size(); }
        AnimGraphInstance* GetAnimGraphInstance(size_t index) const    { MCore::LockGuardRecursive lock(m_animGraphInstanceLock); return m_animGraphInstances[index]; }

        size_t FindAnimGraphInstanceIndex(AnimGraphInstance* animGraphInstance) const;

        void SetAnimGraphVisualizationEnabled(bool enabled);

        void RecursiveCollectObjectsAffectedBy(AnimGraph* animGraph, AZStd::vector<EMotionFX::AnimGraphObject*>& affectedObjects);

    private:
        AZStd::vector<AnimGraph*>           m_animGraphs;
        AZStd::vector<AnimGraphInstance*>   m_animGraphInstances;
        BlendSpaceManager*                  m_blendSpaceManager;
        mutable MCore::MutexRecursive       m_animGraphLock;
        mutable MCore::MutexRecursive       m_animGraphInstanceLock;

        // constructor and destructor
        AnimGraphManager();
        ~AnimGraphManager();
    };
} // namespace EMotionFX
