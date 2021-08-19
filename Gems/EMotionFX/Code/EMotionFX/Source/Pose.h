/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <EMotionFX/Source/PoseData.h>
#include <EMotionFX/Source/Transform.h>


namespace EMotionFX
{
    // forward declarations
    class ActorInstance;
    class Actor;
    class MotionInstance;
    class Node;
    class TransformData;
    class Skeleton;
    class MotionLinkData;

    /**
     *
     *
     */
    class EMFX_API Pose
    {
        MCORE_MEMORYOBJECTCATEGORY(Pose, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_POSE);

    public:
        enum
        {
            FLAG_LOCALTRANSFORMREADY    = 1 << 0,
            FLAG_MODELTRANSFORMREADY    = 1 << 1
        };

        Pose();
        Pose(const Pose& other);
        ~Pose();

        void Clear(bool clearMem = true);
        void ClearFlags(uint8 newFlags = 0);

        void InitFromPose(const Pose* sourcePose);
        void InitFromBindPose(const ActorInstance* actorInstance);
        void InitFromBindPose(const Actor* actor);

        void LinkToActorInstance(const ActorInstance* actorInstance, uint8 initialFlags = 0);
        void LinkToActor(const Actor* actor, uint8 initialFlags = 0, bool clearAllFlags = true);
        void SetNumTransforms(size_t numTransforms);

        void ApplyMorphWeightsToActorInstance();
        void ZeroMorphWeights();

        void UpdateAllLocalSpaceTranforms();
        void UpdateAllModelSpaceTranforms();
        void ForceUpdateFullLocalSpacePose();
        void ForceUpdateFullModelSpacePose();

        const Transform& GetLocalSpaceTransform(size_t nodeIndex) const;
        const Transform& GetModelSpaceTransform(size_t nodeIndex) const;
        Transform GetWorldSpaceTransform(size_t nodeIndex) const;

        void GetLocalSpaceTransform(size_t nodeIndex, Transform* outResult) const;
        void GetModelSpaceTransform(size_t nodeIndex, Transform* outResult) const;
        void GetWorldSpaceTransform(size_t nodeIndex, Transform* outResult) const;

        void SetLocalSpaceTransform(size_t nodeIndex, const Transform& newTransform, bool invalidateModelSpaceTransforms = true);
        void SetModelSpaceTransform(size_t nodeIndex, const Transform& newTransform, bool invalidateChildModelSpaceTransforms = true);
        void SetWorldSpaceTransform(size_t nodeIndex, const Transform& newTransform, bool invalidateChildModelSpaceTransforms = true);

        void UpdateModelSpaceTransform(size_t nodeIndex) const;
        void UpdateLocalSpaceTransform(size_t nodeIndex) const;

        void CompensateForMotionExtraction(EMotionExtractionFlags motionExtractionFlags = (EMotionExtractionFlags)0);
        void CompensateForMotionExtractionDirect(EMotionExtractionFlags motionExtractionFlags = (EMotionExtractionFlags)0);

        /**
         * Use this method when you need to transform the mesh data like vertex positions into world space.
         * Multiply all your vertices with the transform returned by this.
         * The difference between using GetWorldSpaceTransform directly from the current pose is that this looks whether the mesh is skinned or not.
         * Right now we handle skinned meshes differently. This will change in the future. Skinned meshes will always return an identity transform and therefore act like they cannot be animated.
         * This requires the pose to be linked to an actor instance. If this is not the case, identity transform is returned.
         * @param The LOD level, which must be in range of 0..m_actor->GetNumLODLevels().
         * @param nodeIndex The index of the node. If this node happens to have no mesh the regular current world space transform is returned.
         */
        Transform GetMeshNodeWorldSpaceTransform(size_t lodLevel, size_t nodeIndex) const;

        void InvalidateAllLocalSpaceTransforms();
        void InvalidateAllModelSpaceTransforms();
        void InvalidateAllLocalAndModelSpaceTransforms();

        Transform CalcTrajectoryTransform() const;

        MCORE_INLINE const Transform* GetLocalSpaceTransforms() const                               { return m_localSpaceTransforms.data(); }
        MCORE_INLINE const Transform* GetModelSpaceTransforms() const                               { return m_modelSpaceTransforms.data(); }
        MCORE_INLINE size_t GetNumTransforms() const                                                { return m_localSpaceTransforms.size(); }
        MCORE_INLINE const ActorInstance* GetActorInstance() const                                  { return m_actorInstance; }
        MCORE_INLINE const Actor* GetActor() const                                                  { return m_actor; }
        MCORE_INLINE const Skeleton* GetSkeleton() const                                            { return m_skeleton; }

        MCORE_INLINE Transform& GetLocalSpaceTransformDirect(size_t nodeIndex)                      { return m_localSpaceTransforms[nodeIndex]; }
        MCORE_INLINE Transform& GetModelSpaceTransformDirect(size_t nodeIndex)                      { return m_modelSpaceTransforms[nodeIndex]; }
        MCORE_INLINE const Transform& GetLocalSpaceTransformDirect(size_t nodeIndex) const          { return m_localSpaceTransforms[nodeIndex]; }
        MCORE_INLINE const Transform& GetModelSpaceTransformDirect(size_t nodeIndex) const          { return m_modelSpaceTransforms[nodeIndex]; }
        MCORE_INLINE void SetLocalSpaceTransformDirect(size_t nodeIndex, const Transform& transform){ m_localSpaceTransforms[nodeIndex]  = transform; m_flags[nodeIndex] |= FLAG_LOCALTRANSFORMREADY; }
        MCORE_INLINE void SetModelSpaceTransformDirect(size_t nodeIndex, const Transform& transform){ m_modelSpaceTransforms[nodeIndex] = transform; m_flags[nodeIndex] |= FLAG_MODELTRANSFORMREADY; }
        MCORE_INLINE void InvalidateLocalSpaceTransform(size_t nodeIndex)                           { m_flags[nodeIndex] &= ~FLAG_LOCALTRANSFORMREADY; }
        MCORE_INLINE void InvalidateModelSpaceTransform(size_t nodeIndex)                           { m_flags[nodeIndex] &= ~FLAG_MODELTRANSFORMREADY; }

        MCORE_INLINE void SetMorphWeight(size_t index, float weight)                                { m_morphWeights[index] = weight; }
        MCORE_INLINE float GetMorphWeight(size_t index) const                                       { return m_morphWeights[index]; }
        MCORE_INLINE size_t GetNumMorphWeights() const                                              { return m_morphWeights.size(); }
        void ResizeNumMorphs(size_t numMorphTargets);

        /**
         * Blend this pose into a specified destination pose.
         * @param destPose The destination pose to blend into.
         * @param weight The weight value to use.
         * @param instance The motion instance settings to use.
         */
        void Blend(const Pose* destPose, float weight, const MotionInstance* instance);

        /**
         * Blend the transforms for all enabled nodes in the actor instance.
         * @param destPose The destination pose to blend into.
         * @param weight The weight value to use, which must be in range of [0..1], where 1.0 is the dest pose.
         */
        void Blend(const Pose* destPose, float weight);

        /**
         * Additively blend the transforms for all enabled nodes in the actor instance.
         * You can see this as: thisPose += destPose * weight.
         * @param destPose The destination pose to blend into, additively, so displacing the current pose.
         * @param weight The weight value to use, which must be in range of [0..1], where 1.0 is the dest pose.
         */
        void BlendAdditiveUsingBindPose(const Pose* destPose, float weight);

        /**
         * Blend this pose into a specified destination pose.
         * @param destPose The destination pose to blend into.
         * @param weight The weight value to use.
         * @param instance The motion instance settings to use.
         * @param outPose the output pose, in which we will write the result.
         */
        void Blend(const Pose* destPose, float weight, const MotionInstance* instance, Pose* outPose);

        Pose& PreMultiply(const Pose& other);
        Pose& Multiply(const Pose& other);
        Pose& MultiplyInverse(const Pose& other);

        void Zero();
        void NormalizeQuaternions();
        void Sum(const Pose* other, float weight);

        Pose& MakeRelativeTo(const Pose& other);
        Pose& MakeAdditive(const Pose& refPose);
        Pose& ApplyAdditive(const Pose& additivePose);
        Pose& ApplyAdditive(const Pose& additivePose, float weight);

        void Mirror(const MotionLinkData* motionLinkData);

        Pose& operator=(const Pose& other);

        MCORE_INLINE uint8 GetFlags(size_t nodeIndex) const         { return m_flags[nodeIndex]; }
        MCORE_INLINE void SetFlags(size_t nodeIndex, uint8 flags)   { m_flags[nodeIndex] = flags; }

        bool HasPoseData(const AZ::TypeId& typeId) const;
        PoseData* GetPoseDataByType(const AZ::TypeId& typeId) const;
        template <class T>
        T* GetPoseData() const                   { return azdynamic_cast<T*>(GetPoseDataByType(azrtti_typeid<T>())); }

        void AddPoseData(PoseData* poseData);
        void ClearPoseDatas();
        const AZStd::unordered_map<AZ::TypeId, AZStd::unique_ptr<PoseData> >& GetPoseDatas() const;

        /**
         * Guaranteed retrieval of a fully prepared and ready to use pose data of the given type.
         * Pose data of the given type will be created and reset to its default value in case it does not yet exist on the current pose.
         * In case the pose data existed but was unused, the pose data will be reset before being returned.
         * @param[in] typeId The type id of the pose data that we want to retrieve.
         * @param[in] linkToActorInstance The actor instance to link to the pose data.
         * @result Valid pointer to the pose data of the given type.
         */
        PoseData* GetAndPreparePoseData(const AZ::TypeId& typeId, ActorInstance* linkToActorInstance);
        template <class T>
        T* GetAndPreparePoseData(ActorInstance* linkToActorInstance) { return azdynamic_cast<T*>(GetAndPreparePoseData(azrtti_typeid<T>(), linkToActorInstance)); }

    private:
        mutable AZStd::vector<Transform>  m_localSpaceTransforms;
        mutable AZStd::vector<Transform>  m_modelSpaceTransforms;
        mutable AZStd::vector<uint8>      m_flags;
        AZStd::unordered_map<AZ::TypeId, AZStd::unique_ptr<PoseData> > m_poseDatas;
        AZStd::vector<float>              m_morphWeights;      /**< The morph target weights. */
        const ActorInstance*                        m_actorInstance;
        const Actor*                                m_actor;
        const Skeleton*                             m_skeleton;

        void RecursiveInvalidateModelSpaceTransforms(const Actor* actor, size_t nodeIndex);

        /**
         * Perform a non-mixed blend into the specified destination pose.
         * @param destPose The destination pose to blend into.
         * @param weight The weight value to use.
         * @param instance The motion instance settings to use.
         * @param outPose the output pose, in which we will write the result.
         */
        void BlendNonMixed(const Pose* destPose, float weight, const MotionInstance* instance, Pose* outPose);

        /**
         * Perform mixed blend into the specified destination pose.
         * @param destPose The destination pose to blend into.
         * @param weight The weight value to use.
         * @param instance The motion instance settings to use.
         * @param outPose the output pose, in which we will write the result.
         */
        void BlendMixed(const Pose* destPose, float weight, const MotionInstance* instance, Pose* outPose);

        /**
         * Blend a single local space transform into another transform, using additive blending.
         * This will basically do like: outTransform = source + (dest - baseTransform) * weight.
         * @param baseTransform The base pose local transform, used to calculate the additive amount.
         * @param source The source transformation.
         * @param dest The destination transformation.
         * @param weight The weight value, which must be in range of [0..1].
         * @param outTransform The transform object to output the result in.
         */
        void BlendTransformAdditiveUsingBindPose(const Transform& baseTransform, const Transform& source, const Transform& dest, float weight, Transform* outTransform);

        /**
         * Blend a single local space transform into another transform, including weight check.
         * This weight check will check if the weight is 0 or 1, and perform specific optimizations in such cases.
         * @param source The source transformation.
         * @param dest The destination transformation.
         * @param weight The weight value, which must be in range of [0..1].
         * @param outTransform The transform object to output the result in.
         */
        void BlendTransformWithWeightCheck(const Transform& source, const Transform& dest, float weight, Transform* outTransform);
    };
}   // namespace EMotionFX
