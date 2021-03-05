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
#include "MeshDeformer.h"
#include "BaseObject.h"
#include <MCore/Source/Array.h>


namespace EMotionFX
{
    // forward declarations
    class ActorInstance;
    class Actor;
    class Node;


    /**
     * The mesh deformer stack.
     * This class represents a stack of mesh deformers, which are executed on a given mesh in
     * the order defined by the stack. The deformers will be executed from bottom to the top.
     * An example stack could be:
     * - Twist deformer
     * - SoftSkin deformer
     * - Morph deformer
     *
     * This would first perform a morph on the given mesh. After that a softskinning deformer would be applied to it, using
     * bone deformations. And finally the result of that would be deformed by a twist modifier, which would twist the mesh.
     * People who know 3D Studio Max will recognise this system as the Max Modifier Stack.
     */
    class EMFX_API MeshDeformerStack
        : public BaseObject
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        /**
         * Creation method.
         * @param mesh The mesh to apply this deformer on.
         */
        static MeshDeformerStack* Create(Mesh* mesh);

        /**
         * Update the stack calling the mesh deformers.
         * @param actor The actor instance to use for the update. So the actor instance where the stack belongs to during this update.
         * @param node The node to use for the update, so the node where the mesh belongs to during this update.
         * @param timeDelta The time (in seconds) passed since the last call.
         * @param forceUpdateDisabledDeformers When set to true this will force updating disabled deformers.
         */
        void Update(ActorInstance* actor, Node* node, float timeDelta, bool forceUpdateDisabledDeformers=false);

        /**
        * Update the stack calling the mesh morph deformers.
        * @param actor The actor instance to use for the update. So the actor instance where the stack belongs to during this update.
        * @param node The node to use for the update, so the node where the mesh belongs to during this update.
        * @param timeDelta The time (in seconds) passed since the last call.
        * @param typeID The type of the deformer you wish to apply.
        * @param resetMesh Should the mesh be reset before this deformer or not.
        * @param forceUpdateDisabledDeformers When set to true this will force updating disabled deformers.
        */
        void UpdateByModifierType(ActorInstance* actorInstance, Node* node, float timeDelta, uint32 typeID, bool resetMesh, bool forceUpdateDisabledDeformers);

        /**
         * Iterates through all mesh deformers in the stack and reinitializes them.
         * @param actor The actor that will use the mesh deformers.
         * @param node The node to use for the reinitialize, so the node where the mesh belongs to during this initialization.
         * @param lodLevel The LOD level the mesh deformers work on.
         */
        void ReinitializeDeformers(Actor* actor, Node* node, uint32 lodLevel);

        /**
         * Add a given deformer to the back of the stack.
         * @param meshDeformer The deformer to add.
         */
        void AddDeformer(MeshDeformer* meshDeformer);

        /**
         * Insert a given deformer at a given position in the deformer stack.
         * @param pos The position to insert the deformer.
         * @param meshDeformer The deformer to store at this position.
         */
        void InsertDeformer(uint32 pos, MeshDeformer* meshDeformer);

        /**
         * Remove a given deformer.
         * @param meshDeformer The item/element to remove.
         */
        bool RemoveDeformer(MeshDeformer* meshDeformer);

        /**
         * Remove all deformers from this mesh deformer stack that have a specified type ID.
         * So you can use this to for example delete all softskin deformers.
         * @param deformerTypeID The type ID of the deformer, which is returned by MeshDeformer::GetType().
         * @result Returns the number of deformers that have been removed.
         */
        uint32 RemoveAllDeformersByType(uint32 deformerTypeID);

        /**
         * Remove all deformers from this mesh deformer stack.
         */
        void RemoveAllDeformers();

        /**
         * Enable or disable all the deformers with the specified type ID.
         * You can use this to for example disable all softskin deformers.
         * @param deformerTypeID The type ID of the deformer, which is returned by MeshDeformer::GetType().
         * @param enabled Set to true when you want to enable these deformers, or false if you want to disable them.
         * @result Returns the number of deformers that have been enabled or disabled.
         */
        uint32 EnableAllDeformersByType(uint32 deformerTypeID, bool enabled);

        /**
         * Creates an exact clone (copy) of this deformer stack, including all deformers (which will also be cloned).
         * @param mesh The mesh to apply the new stack on.
         * @result A pointer to the cloned stack.
         */
        MeshDeformerStack* Clone(Mesh* mesh);

        /**
         * Returns the mesh we are applying the stack on.
         * @result A pointer to the mesh.
         */
        Mesh* GetMesh() const;

        /**
         * Get the number of deformers in the stack.
         * @result The number of deformers in the stack.
         */
        uint32 GetNumDeformers() const;

        /**
         * Get a given deformer.
         * @param nr The deformer number to get.
         * @result A pointer to the deformer.
         */
        MeshDeformer* GetDeformer(uint32 nr) const;

        /**
         * Check if the stack contains a deformer of a given type.
         * @param deformerTypeID The type ID of the deformer you'd like to check.
         * @result Returns true when the stack has one or more deformers of the specified type, otherwise false is returned.
         */
        bool CheckIfHasDeformerOfType(uint32 deformerTypeID) const;

        /**
         * Find a mesh deformer of a given type as returned by MeshDeformer::GetType().
         * @param deformerTypeID The mesh deformer type to search for.
         * @param occurrence In case there are multiple controllers of the same type, 0 means it returns the first one, 1 means the second, etc.
         * @result A pointer to the mesh deformer of the given type, or nullptr when not found.
         */
        MeshDeformer* FindDeformerByType(uint32 deformerTypeID, uint32 occurrence = 0) const;

    private:
        MCore::Array<MeshDeformer*> mDeformers;     /**< The stack of deformers. */
        Mesh*                       mMesh;          /**< Pointer to the mesh to which the modifier stack belongs to.*/

        /**
         * Constructor.
         * @param mesh The mesh to apply this deformer on.
         */
        MeshDeformerStack(Mesh* mesh);

        /**
         * Destructor.
         */
        ~MeshDeformerStack();
    };
} // namespace EMotionFX
