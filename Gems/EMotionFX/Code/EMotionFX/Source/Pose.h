/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <MCore/Source/AlignedArray.h>
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
        void SetNumTransforms(uint32 numTransforms);

        void ApplyMorphWeightsToActorInstance();
        void ZeroMorphWeights();

        void UpdateAllLocalSpaceTranforms();
        void UpdateAllModelSpaceTranforms();
        void ForceUpdateFullLocalSpacePose();
        void ForceUpdateFullModelSpacePose();

        const Transform& GetLocalSpaceTransform(uint32 nodeIndex) const;
        const Transform& GetModelSpaceTransform(uint32 nodeIndex) const;
        Transform GetWorldSpaceTransform(uint32 nodeIndex) const;

        void GetLocalSpaceTransform(uint32 nodeIndex, Transform* outResult) const;
        void GetModelSpaceTransform(uint32 nodeIndex, Transform* outResult) const;
        void GetWorldSpaceTransform(uint32 nodeIndex, Transform* outResult) const;

        void SetLocalSpaceTransform(uint32 nodeIndex, const Transform& newTransform, bool invalidateModelSpaceTransforms = true);
        void SetModelSpaceTransform(uint32 nodeIndex, const Transform& newTransform, bool invalidateChildModelSpaceTransforms = true);
        void SetWorldSpaceTransform(uint32 nodeIndex, const Transform& newTransform, bool invalidateChildModelSpaceTransforms = true);

        void UpdateModelSpaceTransform(uint32 nodeIndex) const;
        void UpdateLocalSpaceTransform(uint32 nodeIndex) const;

        void CompensateForMotionExtraction(EMotionExtractionFlags motionExtractionFlags = (EMotionExtractionFlags)0);
        void CompensateForMotionExtractionDirect(EMotionExtractionFlags motionExtractionFlags = (EMotionExtractionFlags)0);

        /**
         * Use this method when you need to transform the mesh data like vertex positions into world space.
         * Multiply all your vertices with the transform returned by this.
         * The difference between using GetWorldSpaceTransform directly from the current pose is that this looks whether the mesh is skinned or not.
         * Right now we handle skinned meshes differently. This will change in the future. Skinned meshes will always return an identity transform and therefore act like they cannot be animated.
         * This requires the pose to be linked to an actor instance. If this is not the case, identity transform is returned.
         * @param The LOD level, which must be in range of 0..mActor->GetNumLODLevels().
         * @param nodeIndex The index of the node. If this node happens to have no mesh the regular current world space transform is returned.
         */
        Transform GetMeshNodeWorldSpaceTransform(AZ::u32 lodLevel, AZ::u32 nodeIndex) const;

        void InvalidateAllLocalSpaceTransforms();
        void InvalidateAllModelSpaceTransforms();
        void InvalidateAllLocalAndModelSpaceTransforms();

        Transform CalcTrajectoryTransform() const;

        MCORE_INLINE const Transform* GetLocalSpaceTransforms() const                               { return mLocalSpaceTransforms.GetReadPtr(); }
        MCORE_INLINE const Transform* GetModelSpaceTransforms() const                               { return mModelSpaceTransforms.GetReadPtr(); }
        MCORE_INLINE uint32 GetNumTransforms() const                                                { return mLocalSpaceTransforms.GetLength(); }
        MCORE_INLINE const ActorInstance* GetActorInstance() const                                  { return mActorInstance; }
        MCORE_INLINE const Actor* GetActor() const                                                  { return mActor; }
        MCORE_INLINE const Skeleton* GetSkeleton() const                                            { return mSkeleton; }

        MCORE_INLINE Transform& GetLocalSpaceTransformDirect(uint32 nodeIndex)                      { return mLocalSpaceTransforms[nodeIndex]; }
        MCORE_INLINE Transform& GetModelSpaceTransformDirect(uint32 nodeIndex)                      { return mModelSpaceTransforms[nodeIndex]; }
        MCORE_INLINE const Transform& GetLocalSpaceTransformDirect(uint32 nodeIndex) const          { return mLocalSpaceTransforms[nodeIndex]; }
        MCORE_INLINE const Transform& GetModelSpaceTransformDirect(uint32 nodeIndex) const          { return mModelSpaceTransforms[nodeIndex]; }
        MCORE_INLINE void SetLocalSpaceTransformDirect(uint32 nodeIndex, const Transform& transform){ mLocalSpaceTransforms[nodeIndex]  = transform; mFlags[nodeIndex] |= FLAG_LOCALTRANSFORMREADY; }
        MCORE_INLINE void SetModelSpaceTransformDirect(uint32 nodeIndex, const Transform& transform){ mModelSpaceTransforms[nodeIndex] = transform; mFlags[nodeIndex] |= FLAG_MODELTRANSFORMREADY; }
        MCORE_INLINE void InvalidateLocalSpaceTransform(uint32 nodeIndex)                           { mFlags[nodeIndex] &= ~FLAG_LOCALTRANSFORMREADY; }
        MCORE_INLINE void InvalidateModelSpaceTransform(uint32 nodeIndex)                           { mFlags[nodeIndex] &= ~FLAG_MODELTRANSFORMREADY; }

        MCORE_INLINE void SetMorphWeight(uint32 index, float weight)                                { mMorphWeights[index] = weight; }
        MCORE_INLINE float GetMorphWeight(uint32 index) const                                       { return mMorphWeights[index]; }
        MCORE_INLINE uint32 GetNumMorphWeights() const                                              { return mMorphWeights.GetLength(); }
        void ResizeNumMorphs(uint32 numMorphTargets);

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

        MCORE_INLINE uint8 GetFlags(uint32 nodeIndex) const         { return mFlags[nodeIndex]; }
        MCORE_INLINE void SetFlags(uint32 nodeIndex, uint8 flags)   { mFlags[nodeIndex] = flags; }

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
        mutable MCore::AlignedArray<Transform, 16>  mLocalSpaceTransforms;
        mutable MCore::AlignedArray<Transform, 16>  mModelSpaceTransforms;
        mutable MCore::AlignedArray<uint8, 16>      mFlags;
        AZStd::unordered_map<AZ::TypeId, AZStd::unique_ptr<PoseData> > m_poseDatas;
        MCore::AlignedArray<float, 16>              mMorphWeights;      /**< The morph target weights. */
        const ActorInstance*                        mActorInstance;
        const Actor*                                mActor;
        const Skeleton*                             mSkeleton;

        void RecursiveInvalidateModelSpaceTransforms(const Actor* actor, uint32 nodeIndex);

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
