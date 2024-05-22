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
#include "Node.h"
#include "Actor.h"
#include "MorphTarget.h"
#include <MCore/Source/CompressedVector.h>


namespace EMotionFX
{
    /**
     * The standard  morph target.
     * This type of  morph target supports vertex morphs (position, normals and first tangent layer) as well
     * as transformation morph targets.
     */
    class EMFX_API MorphTargetStandard
        : public MorphTarget
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        // the morph target type ID, returned by GetType()
        enum
        {
            TYPE_ID = 0x00000001
        };

        /**
         * The memory block ID where allocations made by this class will remain in.
         */
        enum
        {
            MEMORYBLOCK_ID = 101
        };

        /**
         * The structure which contains the deformation data for each node which is being touched by this morph target.
         */
        class DeformData
            : public MCore::RefCounted
        {
        public:
            AZ_CLASS_ALLOCATOR_DECL

            /**
             * Relative (delta) vertex data offset values.
             * This can be used in context of: "newVertex += vertexDelta * morphTargetWeight"
             */
            struct EMFX_API VertexDelta
            {
                MCore::Compressed16BitVector3   m_position;          /**< The position delta. */
                MCore::Compressed8BitVector3    m_normal;            /**< The normal delta. */
                MCore::Compressed8BitVector3    m_tangent;           /**< The first tangent layer delta. */
                MCore::Compressed8BitVector3    m_bitangent;         /**< The first bitangent layer delta. */
                uint32                          m_vertexNr;          /**< The vertex number inside the mesh to apply this to. */
            };

            static DeformData* Create(size_t nodeIndex, uint32 numVerts);

            // creates a clone
            DeformData* Clone();

        public:
            VertexDelta*    m_deltas;            /**< The delta values. */
            uint32          m_numVerts;          /**< The number of vertices in the m_deltas and m_vertexNumbers arrays. */
            size_t          m_nodeIndex;         /**< The node which this data works on. */
            float           m_minValue;          /**< The compression/decompression minimum value for the delta positions. */
            float           m_maxValue;          /**< The compression/decompression maximum value for the delta positions. */

            /**
             * The constructor.
             * @param nodeIndex The node number on which the deformations should work.
             * @param numVerts The number of vertices modified by this deform.
             */
            DeformData(size_t nodeIndex, uint32 numVerts);

            /**
             * The destructor.
             * Automatically releases all allocated memory by this deform.
             */
            ~DeformData();
        };

        /**
         * The transformation struct.
         * This contains a position, rotation and scale.
         * Please keep in mind that the rotation is stored as non-delta value, while the position and scale are
         * stored as delta values.
         */
        struct EMFX_API alignas(16) Transformation
        {
            AZ::Quaternion      m_rotation;          /**< The rotation as absolute value. So not a delta value, but a target (absolute) rotation. */
            AZ::Quaternion      m_scaleRotation;     /**< The scale rotation, as absolute value. */
            AZ::Vector3         m_position;          /**< The position as a delta, so the difference between the original and target position. */
            AZ::Vector3         m_scale;             /**< The scale as a delta, so the difference between the original and target scale. */
            size_t              m_nodeIndex;         /**< The node number to apply this on. */
        };

        /**
         * The constructor.
         * Please note that you will have to call the InitFromPose method in order to make it do something. Or use the extended constructor.
         * @param name The unique name of the morph target.
         */
        static MorphTargetStandard* Create(const char* name);

        /**
         * Extended constructor.
         * @param captureTransforms Set this to true if you want this morph target to capture rigid transformations (changes in pos/rot/scale).
         * @param neutralPose The actor that contains the neutral pose.
         * @param targetPose The actor representing the pose of the character when the weight value would equal 1.
         * @param name The unique name of the morph target.
         */
        static MorphTargetStandard* Create(bool captureTransforms, Actor* neutralPose, Actor* targetPose, const char* name);

        /**
         * Get the type of morph target.
         * You can have different types of morph targets, such as morph targets which work
         * with bones, or which work with morphing or other techniques.
         * @result The unique type ID of the morph target.
         */
        uint32 GetType() const override;

        /**
         * Initializes this expresion part from two given actors, representing a neutral and target pose.
         * The morph target will filter out all data which is changed compared to the base pose and
         * store this information on a specific way so it can be used to accumulate multiple morph targets
         * together and apply them to the actor to which this morph target is attached to.
         * @param captureTransforms Set this to true if you want this morph target to capture rigid transformations (changes in pos/rot/scale).
         * @param neutralPose The actor that represents the neutral pose.
         * @param targetPose The actor representing the pose of the character when the weight value would equal 1.
         */
        void InitFromPose(bool captureTransforms, Actor* neutralPose, Actor* targetPose) override;

        /**
         * Apply the relative transformation caused by this morph target to a given node.
         * It does not change the node itself, but it modifies the input position, rotation and scale.
         * The node and actor pointers are only needed to retrieve some additional information from the node, needed
         * to perform the calculations.
         * @param actorInstance The actor instance where the node belongs to.
         * @param nodeIndex The node we would like to modify (please note that the node itself will stay untouched).
         * @param position The input position to which relative adjustments will be applied.
         * @param rotation The input rotation to which relative adjustments will be applied.
         * @param scale The input scale to which relative adjustments will be applied.
         * @param weight The absolute weight value.
         */
        void ApplyTransformation(ActorInstance* actorInstance, size_t nodeIndex, AZ::Vector3& position, AZ::Quaternion& rotation, AZ::Vector3& scale, float weight) override;

        /**
         * Checks if this morph target would influence the given node.
         * @param nodeIndex The node to perform the check with.
         * @result Returns true if the given node will be modified by this morph target, otherwise false is returned.
         */
        bool Influences(size_t nodeIndex) const override;

        /**
         * Apply the relative deformations for this morph target to the given actor instance.
         * @param actorInstance The actor instance to apply the deformations on.
         * @param weight The absolute weight of the morph target.
         */
        void Apply(ActorInstance* actorInstance, float weight) override;

        /**
         * Get the number of deform data objects.
         * @result The number of deform data objects.
         */
        size_t GetNumDeformDatas() const;

        /**
         * Get a given deform data object.
         * @param nr The deform data number, which must be in range of [0..GetNumDeformDatas()-1].
         * @result A pointer to the deform data object.
         */
        DeformData* GetDeformData(size_t nr) const;

        /**
         * Add a given deform data to the array of deform data objects.
         * @param data The deform data object to add.
         */
        void AddDeformData(DeformData* data);

        /**
         * Add a new transformation to the morph target for the given node.
         * @param transform The transformation which contains the relative position, scale and the absolute rotation and the node index to apply this on.
         */
        void AddTransformation(const Transformation& transform);

        /**
         * Get the number of transformations which are part of this bones morph target.
         * @result The number of tranformations.
         */
        size_t GetNumTransformations() const;

        /**
         * Get a given transformation and it's corresponding node id to which the transformation belongs to.
         * @param nr The transformation number, must be in range of [0..GetNumTransformations()-1].
         * @result A reference to the transformation.
         */
        Transformation& GetTransformation(size_t nr);

        /**
         * Creates an exact clone of this  morph target.
         * @result Returns a pointer to an exact clone of this morph target.
         */
        MorphTarget* Clone() const override;

        /**
         * Remove all deform data objects from memory as well as from the class.
         */
        void RemoveAllDeformDatas();

        /**
         * Remove all deform data objects for the given joint.
         */
        void RemoveAllDeformDatasFor(Node* joint);

        /**
         * Remove the given deform data.
         * @param index The deform data to remove. The index must be in range of [0, GetNumDeformDatas()].
         * @param delFromMem Set to true (default) when you wish to also delete the specified deform data from memory.
         */
        void RemoveDeformData(size_t index, bool delFromMem = true);

        /**
         * Remove the given transformation.
         * @param index The transformation to remove. The index must be in range of [0, GetNumTransformations()].
         */
        void RemoveTransformation(size_t index);

        /**
         * Reserve (pre-allocate) space in the array of deform datas.
         * This does NOT change the value returned by GetNumDeformDatas().
         * @param numDeformDatas The absolute number of deform datas to pre-allocate space for.
         */
        void ReserveDeformDatas(size_t numDeformDatas);

        /**
         * Reserve (pre-allocate) space in the array of transformations.
         * This does NOT change the value returned by GetNumTransformations().
         * @param numTransforms The absolute number of transformations to pre-allocate space for.
         */
        void ReserveTransformations(size_t numTransforms);

        /**
         * Scale all transform and positional data.
         * This is a very slow operation and is used to convert between different unit systems (cm, meters, etc).
         * @param scaleFactor The scale factor to scale the current data by.
         */
        void Scale(float scaleFactor) override;

    private:
        AZStd::vector<Transformation>   m_transforms;            /**< The relative transformations for the given nodes, in local space. The rotation however is absolute. */
        AZStd::vector<DeformData*>      m_deformDatas;           /**< The deformation data objects. */

        /**
         * The constructor.
         * Please note that you will have to call the InitFromPose method in order to make it do something. Or use the extended constructor.
         * @param name The unique name of the morph target.
         */
        MorphTargetStandard(const char* name);

        /**
         * Extended constructor.
         * @param captureTransforms Set this to true if you want this morph target to capture rigid transformations (changes in pos/rot/scale).
         * @param neutralPose The actor that contains the neutral pose.
         * @param targetPose The actor representing the pose of the character when the weight value would equal 1.
         * @param name The unique name of the morph target.
         */
        MorphTargetStandard(bool captureTransforms, Actor* neutralPose, Actor* targetPose, const char* name);

        /**
         * The destructor.
         */
        ~MorphTargetStandard();
    };
} // namespace EMotionFX
