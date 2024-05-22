/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include "EMotionFXConfig.h"
#include <MCore/Source/RefCounted.h>
#include "Pose.h"

namespace EMotionFX
{
    // forward declarations
    class ActorInstance;

    /**
     * The transformation data class.
     * This class holds all transformation data for each node.
     * This includes local space transforms, local space matrices as well as world space matrices.
     * If for example you wish to get the world space matrices for all nodes, to be used for rendering, you will have to use this class.
     */
    class EMFX_API TransformData
        : public MCore::RefCounted
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        /**
         * Some flags per node, that allow specific optimizations inside EMotion FX.
         */
        enum ENodeFlags
        {
            FLAG_HASSCALE   = 1 << 0,   /**< Has the node a scale factor? */
        };

        //---------------------------------------------------------------------

        /**
         * The creation method.
         */
        static TransformData* Create();

        /**
         * Initialize the transformation data for a given ActorInstance object.
         * This will allocate data for the number of nodes in the actor.
         * You can call this multiple times if needed.
         * @param actorInstance The actor instance to initialize for.
         */
        void InitForActorInstance(ActorInstance* actorInstance);

        /**
         * Release all allocated memory.
         */
        void Release();

        /**
         * Get the skinning matrices (offset from the pose).
         * The size of the returned array is equal to the amount of nodes in the actor or the value returned by GetNumTransforms()
         * @result The array of skinning matrices.
         */
        MCORE_INLINE AZ::Matrix3x4* GetSkinningMatrices() { return m_skinningMatrices; }

        /**
         * Get the skinning matrices (offset from the pose), in read-only (const) mode.
         * The size of the returned array is equal to the amount of nodes in the actor or the value returned by GetNumTransforms()
         * @result The array of skinning matrices.
         */
        MCORE_INLINE const AZ::Matrix3x4* GetSkinningMatrices() const { return m_skinningMatrices; }

        MCORE_INLINE Pose* GetBindPose() const                                                          { return m_bindPose; }
        MCORE_INLINE const Pose* GetCurrentPose() const                                                 { return &m_pose; }
        MCORE_INLINE Pose* GetCurrentPose()                                                             { return &m_pose; }

        /**
         * Reset the local space transform of a given node to its bind pose local space transform.
         * @param nodeIndex The node number, which must be in range of [0..GetNumTransforms()-1].
         */
        void ResetToBindPoseTransformation(size_t nodeIndex)                                            { m_pose.SetLocalSpaceTransform(nodeIndex, m_bindPose->GetLocalSpaceTransform(nodeIndex)); }

        /**
         * Reset all local space transforms to the local space transforms of the bind pose.
         */
        void ResetToBindPoseTransformations()
        {
            for (size_t i = 0; i < m_numTransforms; ++i)
            {
                m_pose.SetLocalSpaceTransform(i, m_bindPose->GetLocalSpaceTransform(i));
            }
        }

        EMFX_SCALECODE
        (
            void SetBindPoseLocalScaleInherit(size_t nodeIndex, const AZ::Vector3& scale);
            void SetBindPoseLocalScale(size_t nodeIndex, const AZ::Vector3& scale);
        )

        MCORE_INLINE const ActorInstance* GetActorInstance() const      { return m_pose.GetActorInstance(); }
        MCORE_INLINE size_t GetNumTransforms() const                    { return m_numTransforms; }

        void MakeBindPoseTransformsUnique();

        void SetNumMorphWeights(size_t numMorphWeights);


    private:
        Pose            m_pose;                  /**< The current pose. */
        Pose*           m_bindPose;              /**< The bind pose, which can be unique or point to the bind pose in the actor. */
        AZ::Matrix3x4*  m_skinningMatrices;      /**< The matrices used for skinning. They are the offset to the bind pose. */
        size_t          m_numTransforms;         /**< The number of transforms, which is equal to the number of nodes in the linked actor instance. */
        bool            m_hasUniqueBindPose;     /**< Do we have a unique bind pose (when set to true) or do we use the one from the Actor object (when set to false)? */

        /**
         * The constructor.
         */
        TransformData();

        /**
         * The destructor.
         */
        ~TransformData();
    };
}   // namespace EMotionFX

