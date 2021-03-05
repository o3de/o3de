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
#include "BaseObject.h"


namespace EMotionFX
{
    // forward declarations
    class Mesh;
    class Actor;
    class Node;
    class ActorInstance;


    /**
     * The mesh deformer base class.
     * A mesh deformer can deform (apply modifications) on a given mesh.
     * Examples of deformers could be a TwistDeformer, SoftSkinDeformer, MorphDeformer, etc.
     * Every deformer has its own unique type ID number and the MeshDeformerStack contains a list of
     * deformers which are executed in the specified order.
     */
    class EMFX_API MeshDeformer
        : public BaseObject
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        /**
         * Update the mesh deformer.
         * @param actorInstance The actor instance to use for the update. So the actor where the mesh belongs to during this update.
         * @param node The node to use for the update, so the node where the mesh belongs to during this update.
         * @param timeDelta The time (in seconds) passed since the last call.
         */
        virtual void Update(ActorInstance* actorInstance, Node* node, float timeDelta) = 0;

        /**
         * Reinitialize the mesh deformer.
         * @param actor The actor that will use the deformer.
         * @param node The node where the mesh belongs to during this initialization.
         * @param lodLevel The LOD level of the mesh the mesh deformer works on.
         */
        virtual void Reinitialize(Actor* actor, Node* node, uint32 lodLevel);

        /**
         * Creates an exact clone (copy) of this deformer, and returns a pointer to it.
         * @param mesh The mesh to apply the cloned deformer on.
         * @result A pointer to the newly created clone of this deformer.
         */
        virtual MeshDeformer* Clone(Mesh* mesh) = 0;

        /**
         * Returns the type identification number of the deformer class.
         * @result The type identification number.
         */
        virtual uint32 GetType() const = 0;

        /**
         * Returns the sub type identification number.
         * This number is used to identify special specilizations of a given deformer, like the same type of deformer, but with P4 optimizations.
         * In that case the type will be the same, but the subtype will be different for each specialization.
         * @result The unique subtype identification number.
         */
        virtual uint32 GetSubType() const = 0;

        /**
         * Check if the controller is enabled or not.
         * @result Returns true when the controller is active (enabled) or false when the controller is inactive (disabled).
         */
        bool GetIsEnabled() const;

        /**
         * Enable or disable the controller.
         * Disabling a controller just results in the Update method of the controller not being called during the Actor::Update() call.
         * @param enabled Set to true when you want to enable the controller or to false when you want to disable the controller.
         */
        void SetIsEnabled(bool enabled);

    protected:
        Mesh*   mMesh;      /**< Pointer to the mesh to which the deformer belongs to.*/
        bool    mIsEnabled; /**< When set to true, this mesh deformer will be processed, otherwise it will be skipped during update. */

        /**
         * Default constructor.
         * @param mesh A pointer to the mesh to deform.
         */
        MeshDeformer(Mesh* mesh);

        /**
         * Destructor.
         */
        virtual ~MeshDeformer();
    };
} // namespace EMotionFX
