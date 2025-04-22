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
#include "MotionSystem.h"


namespace EMotionFX
{
    // forward declarations
    class LayerPass;
    class RepositioningLayerPass;

    /**
     * The layered motion system class.
     * This class performs the blending, mixing and motion management for us.
     * The system contains a set of layers, which are linked together in a hierarchy.
     * The current implementation however manages the system on such a way that we get a stack of layers.
     * The following diagram gives an example of the layout of layers inside this system:
     *
     * <pre>
     *
     *   FINAL OUTPUT
     *        ^
     *        |
     *        |
     *   75%  |  25%
     * ---------------
     * | lay2 | run  |  layer 1 (root layer)
     * ---------------
     *   \
     *    \
     *     \
     *  30% \   70%
     * ------\--------
     * | jump | walk |  layer 2
     * ---------------
     *   \
     *    \
     *     \
     *      \   100%
     * ------\--------
     * |      | jump |  layer 3
     * ---------------
     * </pre>
     *
     * As you can see a layer consists of 2 inputs and a weight factor between these two inputs.
     * An input can either be a motion or the output of another layer.
     * When a motion is being played we don't have to bother about the adding and removing of layers
     * and things such as smooth transitions etc. This is all performed by this system.
     */
    class EMFX_API MotionLayerSystem
        : public MotionSystem
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        // the motion system type, returned by GetType()
        enum
        {
            TYPE_ID = 0x00000001
        };

        /**
         * Creation method.
         * @param actorInstance The actor instance where this layer belongs to.
         */
        static MotionLayerSystem* Create(ActorInstance* actorInstance);

        /**
         * Update this character motions, if updateNodes is false only time values are updated.
         * If it's true, the heavy calcs are done.
         * NOTE: This method is automatically called by the Update method.
         * @param timePassed The time passed since the last call.
         * @param updateNodes If true the nodes will be updated.
         */
        void Update(float timePassed, bool updateNodes) override;

        /**
         * Recursively search for the first non mixing motion and return the motion instance.
         * @result A pointer to the motion instance.
         */
        MotionInstance* FindFirstNonMixingMotionInstance() const override;

        /**
         * Remove all motion layers below the current one.
         * @param source The layer to remove all layers below from. So this does not remove the source layer itself.
         * @result Returns the number of removed layers.
         */
        size_t RemoveLayersBelow(MotionInstance* source);

        /**
         * Update the motion tree.
         * This removes all motion instances that faded out or get overwritten etc.
         */
        void UpdateMotionTree();

        /**
         * Get the unique motion system type ID.
         * @result The motion system type identification number.
         */
        uint32 GetType() const override;

        /**
         * Get the type identification string.
         * This can be a description or the class name of the motion system.
         * @result A pointer to the string containing the name.
         */
        const char* GetTypeString() const override;

        /**
         * Find the location where to insert a motion layer with a given priority level.
         * When InvalidIndex is returned, it needs to be inserted at the bottom of the motion tree.
         * @param priorityLevel The priority level of the motion instance you want to insert.
         * @result The insert pos in the list of motion instances, or InvalidIndex when the new layer has to be inserted at the bottom of the tree.
         */
        size_t FindInsertPos(size_t priorityLevel) const;

        /**
         * Remove all layer passes.
         * @param delFromMem When set to true, the layer passes will also be deleted from memory.
         */
        void RemoveAllLayerPasses(bool delFromMem = true);

        /**
         * Add a layer pass. This layer will be added on the back, so processed as last.
         * @param newPass The new pass to add.
         */
        void AddLayerPass(LayerPass* newPass);

        /**
         * Get the number of layer passes currently added to this motion layer system.
         * @result The number of layer passes.
         */
        size_t GetNumLayerPasses() const;

        /**
         * Remove a given layer pass by index.
         * @param nr The layer pass number to remove.
         * @param delFromMem When set to true, the layer passes will also be deleted from memory.
         */
        void RemoveLayerPass(size_t nr, bool delFromMem = true);

        /**
         * Remove a given layer pass by pointer.
         * @param pass A pointer to the layer pass to remove.
         * @param delFromMem When set to true, the layer passes will also be deleted from memory.
         */
        void RemoveLayerPass(LayerPass* pass, bool delFromMem = true);

        /**
         * Insert a layer pass in the array of layer passes.
         * @param insertPos The index position to insert the layer pass.
         * @param pass The layer pass to insert.
         */
        void InsertLayerPass(size_t insertPos, LayerPass* pass);

        /**
         * Deletes the motion based actor repositioning layer pass, which is always there on default.
         * Please keep in mind that this one is actually stored outside of the array with layer passes that you can add new layers to.
         * If you want to use your own repositioning code, you can disable the built-in one with this method, and add your own repositioning layer.
         */
        void RemoveRepositioningLayerPass();

        /**
         * Get the pointer to a given layer pass.
         * @param index The layer pass number, which must be in range of [0..GetNumLayerPasses()-1].
         * @result A pointer to the layer pass object.
         */
        LayerPass* GetLayerPass(size_t index) const;


    private:
        AZStd::vector<LayerPass*>    m_layerPasses;           /**< The layer passes. */
        RepositioningLayerPass*     m_repositioningPass;     /**< The motion based actor repositioning layer pass. */

        /**
         * Constructor.
         * @param actorInstance The actor instance where this layer belongs to.
         */
        MotionLayerSystem(ActorInstance* actorInstance);

        /**
         * Destructor.
         */
        ~MotionLayerSystem();

        /**
         * Update the nodes.
         */
        void UpdateNodes();

        /**
         * Start playing the specified motion on this actor.
         * The difference with PlayMotion is that PlayMotion takes care of the calls to CreateMotionInstance, and handles the insertion into the motion queue, etc.
         * This method should purely start the motion.
         * @param motion The motion to play.
         * @param info A pointer to an object containing playback information. This pointer is NOT allowed to be nullptr here.
         * @result A pointer to a created motion instance object. You can use this motion instance object to adjust and retrieve playback information at any time.
         * @see PlayBackInfo
         */
        void StartMotion(MotionInstance * motion, class PlayBackInfo * info) override;
    };
} // namespace EMotionFX
