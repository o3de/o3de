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

// include the required headers
#include "EMotionFXConfig.h"
#include "Mesh.h"
#include "MeshDeformer.h"
#include "MemoryCategories.h"


namespace EMotionFX
{
    // forward declarations
    class Actor;
    class Node;
    class MorphTargetStandard;


    /**
     *
     * The mesh morph deformer.
     * This works together with the  morph setup (MorphSetup).
     * Basically what it does is:
     * output = input + morphTargetA*weightA + morphTargetB*weightB + morphTargetC*weightC.... etc.
     */
    class EMFX_API MorphMeshDeformer
        : public MeshDeformer
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        // the unique type ID of this deformer, returned by GetType()
        enum
        {
            TYPE_ID    = 0x00000002
        };

        // the subtype id, returned by GetSubType()
        enum
        {
            SUBTYPE_ID = 0x00000001
        };

        /**
         * A deform pass.
         * This basically links the mesh with each morph target that is being applied on this mesh.
         */
        struct EMFX_API DeformPass
        {
            MorphTargetStandard*    mMorphTarget;   /**< The morph target working on the mesh. */
            uint32                  mDeformDataNr;  /**< An index inside the deform datas of the standard morph target. */
            bool                    mLastNearZero;  /**< Was the last frame's weight near zero? */

            /**
             * Constructor.
             * Automatically initializes on defaults.
             */
            DeformPass()
                : mMorphTarget(nullptr)
                , mDeformDataNr(MCORE_INVALIDINDEX32)
                , mLastNearZero(false) {}
        };

        /**
         * Creation method.
         * @param mesh A pointer to the mesh to deform.
         */
        static MorphMeshDeformer* Create(Mesh* mesh);

        /**
         * Get the unique deformer type ID.
         * @result The unique ID of this deformer type.
         */
        uint32 GetType() const override;

        /**
         * Returns the unique subtype ID of this deformer.
         * A subtype identifies the specialization type of the given deformer.
         * A cplus plus version of a deformer could have a sub type of 0, while the SSE assembly optimized
         * version would have a sub type of 1 for example.
         * @result The sub type identification number.
         */
        uint32 GetSubType() const override;

        /**
         * Update the mesh deformer.
         * @param actorInstance The actor instance to use for the update. So the actor where the mesh belongs to during this update.
         * @param node The node to use for the update, so the node where the mesh belongs to during this update.
         * @param timeDelta The time (in seconds) passed since the last call.
         */
        void Update(ActorInstance* actorInstance, Node* node, float timeDelta) override;

        /**
         * Reinitialize the mesh deformer.
         * @param actor The actor to to initialize the deformer for.
         * @param node The node where the mesh belongs to during this initialization.
         * @param lodLevel The LOD level of the mesh the mesh deformer works on.
         */
        void Reinitialize(Actor* actor, Node* node, uint32 lodLevel) override;

        /**
         * Creates an exact clone (copy) of this deformer, and returns a pointer to it.
         * @param mesh The mesh to apply the deformer on.
         * @result A pointer to the newly created clone of this deformer.
         */
        MeshDeformer* Clone(Mesh* mesh) override;

        /**
         * Add a deform pass.
         * @param deformPass The deform pass to add.
         */
        void AddDeformPass(const DeformPass& deformPass);

        /**
         * Get the number of deform passes.
         * @result The number of deform passes.
         */
        uint32 GetNumDeformPasses() const;

        /**
         * Pre-allocate space for the deform passes.
         * This does not influence the return value of GetNumDeformPasses().
         * @param numPasses The number of passes to pre-allocate space for.
         */
        void ReserveDeformPasses(uint32 numPasses);

    private:
        MCore::Array<DeformPass>    mDeformPasses;  /**< The deform passes. Each pass basically represents a morph target. */

        /**
         * Default constructor.
         * @param mesh A pointer to the mesh to deform.
         */
        MorphMeshDeformer(Mesh* mesh);

        /**
         * Destructor.
         */
        ~MorphMeshDeformer();
    };
} // namespace EMotionFX
