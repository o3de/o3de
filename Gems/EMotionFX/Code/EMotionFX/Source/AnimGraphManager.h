/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include "EMotionFXConfig.h"
#include <AzCore/std/containers/vector.h>
#include "BaseObject.h"
#include <MCore/Source/Array.h>
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
        : public BaseObject
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        static AnimGraphManager* Create();

        void Init();

        MCORE_INLINE BlendSpaceManager* GetBlendSpaceManager() const { return mBlendSpaceManager; }

        // anim graph helper functions
        void AddAnimGraph(AnimGraph* setup);
        void RemoveAnimGraph(size_t index, bool delFromMemory = true);
        bool RemoveAnimGraph(AnimGraph* animGraph, bool delFromMemory = true);
        void RemoveAllAnimGraphs(bool delFromMemory = true);

        MCORE_INLINE uint32 GetNumAnimGraphs() const                                { MCore::LockGuardRecursive lock(mAnimGraphLock); return static_cast<uint32>(mAnimGraphs.size()); }
        MCORE_INLINE AnimGraph* GetAnimGraph(uint32 index) const                    { MCore::LockGuardRecursive lock(mAnimGraphLock); return mAnimGraphs[index]; }
        AnimGraph* GetFirstAnimGraph() const;

        uint32 FindAnimGraphIndex(AnimGraph* animGraph) const;
        AnimGraph* FindAnimGraphByFileName(const char* filename, bool isTool = true) const;
        AnimGraph* FindAnimGraphByID(uint32 animGraphID) const;

        // anim graph instance helper functions
        void AddAnimGraphInstance(AnimGraphInstance* animGraphInstance);
        void RemoveAnimGraphInstance(size_t index, bool delFromMemory = true);
        bool RemoveAnimGraphInstance(AnimGraphInstance* animGraphInstance, bool delFromMemory = true);
        void RemoveAnimGraphInstances(AnimGraph* animGraph, bool delFromMemory = true);
        void RemoveAllAnimGraphInstances(bool delFromMemory = true);
        void UpdateInstancesUniqueDataUsingMotionSet(EMotionFX::MotionSet* motionSet);

        size_t GetNumAnimGraphInstances() const                        { MCore::LockGuardRecursive lock(mAnimGraphInstanceLock); return mAnimGraphInstances.size(); }
        AnimGraphInstance* GetAnimGraphInstance(size_t index) const    { MCore::LockGuardRecursive lock(mAnimGraphInstanceLock); return mAnimGraphInstances[index]; }

        uint32 FindAnimGraphInstanceIndex(AnimGraphInstance* animGraphInstance) const;

        void SetAnimGraphVisualizationEnabled(bool enabled);

        void RecursiveCollectObjectsAffectedBy(AnimGraph* animGraph, AZStd::vector<EMotionFX::AnimGraphObject*>& affectedObjects);

    private:
        AZStd::vector<AnimGraph*>           mAnimGraphs;
        AZStd::vector<AnimGraphInstance*>   mAnimGraphInstances;
        BlendSpaceManager*                  mBlendSpaceManager;
        mutable MCore::MutexRecursive       mAnimGraphLock;
        mutable MCore::MutexRecursive       mAnimGraphInstanceLock;

        // constructor and destructor
        AnimGraphManager();
        ~AnimGraphManager();
    };
} // namespace EMotionFX