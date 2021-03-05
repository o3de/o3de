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
#include "VertexAttributeLayer.h"
#include <MCore/Source/Array2D.h>


namespace EMotionFX
{
    /**
     * A soft skinning influence between a vertex and a bone, with a given weight.
     */
    class EMFX_API SkinInfluence
    {
        MCORE_MEMORYOBJECTCATEGORY(SkinInfluence, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_GEOMETRY_VERTEXATTRIBUTES);

    public:
        /**
         * Default constructor.
         */
        SkinInfluence()
            : mWeight(0.0f)
            , mBoneNr(0)
            , mNodeNr(0) {}

        /**
         * Constructor.
         * @param nodeNr The node number which acts as bone.
         * @param weight The weight value, which must be in range of [0..1].
         * @param boneNr The bone number, used as optimization inside the softskin deformer.
         */
        SkinInfluence(uint16 nodeNr, float weight, uint16 boneNr = 0)
            : mWeight(weight)
            , mBoneNr(boneNr)
            , mNodeNr(nodeNr) {}

        /**
         * Get the weight of this influence.
         * @result The weight, which should be in range of [0..1].
         */
        MCORE_INLINE float GetWeight() const                                    { return mWeight; }

        /**
         * Adjust the weight value.
         * @param weight The weight value, which must be in range of [0..1].
         */
        void SetWeight(float weight)                                            { mWeight = weight; }

        /**
         * Get the node number that points inside an actor.
         * So this number is an index you can pass to Actor::GetNode(...) to get the actual node that acts as bone.
         * @result The node number, which points inside the nodes array of the actor.
         */
        MCORE_INLINE uint16 GetNodeNr() const                                   { return mNodeNr; }

        /**
         * Set the node number that points inside an actor.
         * So this number is an index you can pass to Actor::GetNode(...) to get the actual node that acts as bone.
         * @param nodeNr The node number, which points inside the nodes array of the actor.
         */
        void SetNodeNr(uint16 nodeNr)                                           { mNodeNr = nodeNr; }

        /**
         * Set the bone number, used for precalculations.
         * @param boneNr The bone number.
         */
        void SetBoneNr(uint16 boneNr)                                           { mBoneNr = boneNr; }

        /**
         * Get the bone number, which is used for precalculations.
         * @result The bone number.
         */
        MCORE_INLINE uint16 GetBoneNr() const                                   { return mBoneNr; }

    private:
        float   mWeight;    /**< The weight value, between 0 and 1. */
        uint16  mBoneNr;    /**< A bone number, which points in an array of bone info structs used for precalculating the skinning matrices. */
        uint16  mNodeNr;    /**< The node number inside the actor which acts as a bone. */
    };


    /**
     * The vertex attribute layer that contains UV texture coordinates.
     * This layer represents an entire UV mapping channel.
     */
    class EMFX_API SkinningInfoVertexAttributeLayer
        : public VertexAttributeLayer
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        enum
        {
            TYPE_ID = 0x00000003
        };

        /**
         * The create method.
         * @param numAttributes The number of attributes to store inside this layer.
         * @param allocData When set to true, it will already allocate space for the amount of attributes.
         */
        static SkinningInfoVertexAttributeLayer* Create(uint32 numAttributes, bool allocData = true);

        /**
         * Get the unique layer type.
         * This identifies what type of attributes are stored internally.
         * An example could be the type ID of an UV attribute layer, or a layer with colors or one which
         * identifies a layer that contains softskinning information.
         * @result The unique type ID, which identifies what type of data is stored inside this layer. Each class inherited from
         *         the VertexAttributeLayer class requires a unique type ID.
         */
        uint32 GetType() const override;

        /**
         * Get the description of the vertex attributes or layer.
         * You most likely want this to be the class name.
         * @result A pointer to the string containing the name or description of the type of vertex attributes of this layer.
         */
        const char* GetTypeString() const override;

        /**
         * Add a given influence (using a bone and a weight).
         * @param attributeNr The attribute/vertex number.
         * @param nodeNr The node in the actor that will be the bone.
         * @param weight The weight to use in the influence.
         * @param boneNr The bone number, used for optimizations inside the softskin deformer.
         */
        void AddInfluence(uint32 attributeNr, uint32 nodeNr, float weight, uint32 boneNr = 0);

        /**
         * Remove the given influence.
         * The influences won't be deleted from memory. To get rid of the not used memory call OptimizeMemoryUsage().
         * @param attributeNr The attribute/vertex number.
         * @param influenceNr The influence number
         */
        void RemoveInfluence(uint32 attributeNr, uint32 influenceNr);

        /**
         * Get the number of influences.
         * @param attributeNr The attribute/vertex number.
         * @result The number of influences.
         */
        MCORE_INLINE uint32 GetNumInfluences(uint32 attributeNr)                                            { return mData.GetNumElements(attributeNr); }

        /**
         * Get a given influence.
         * @param attributeNr The attribute/vertex number.
         * @param influenceNr The influence number, which must be in range of [0..GetNumInfluences()]
         * @result The given influence.
         */
        MCORE_INLINE SkinInfluence* GetInfluence(uint32 attributeNr, uint32 influenceNr)                    { return &mData.GetElement(attributeNr, influenceNr); }

        /**
         * Get direct access to the jagged 2D array that contains the skinning influence data.
         * This can be used in the importers for fast loading and not having to add influence per influence.
         * @result A reference to the 2D array containing all the skinning influences.
         */
        MCORE_INLINE MCore::Array2D<SkinInfluence>& GetArray2D()                                            { return mData; }

        /**
         * Clone the vertex attribute layer.
         * @result A clone of this layer.
         */
        VertexAttributeLayer* Clone() override;

        /**
         * Swap the data for two attributes.
         * You specify two attribute numbers, the data for them should be swapped.
         * This is used for our  mesh system and will be called by Mesh::SwapVertex as well.
         * @param attribA The first attribute number.
         * @param attribB The second attribute number.
         */
        void SwapAttributes(uint32 attribA, uint32 attribB) override;

        /**
         * Remap all influences from an old bone to a new bone. This will overwrite all influences linked to the old node
         * and replace the link to the new node.
         * @param oldNodeNr The node to search and replace all influence links.
         * @param newNodeNr The node with which all found influence links will be replaced with.
         */
        void RemapInfluences(uint32 oldNodeNr, uint32 newNodeNr);

        /**
         * Remove a range of attributes.
         * @param startAttributeNr The start attribute number.
         * @param endAttributeNr The end attribute number, which will also be removed.
         */
        void RemoveAttributes(uint32 startAttributeNr, uint32 endAttributeNr) override;

        /**
         * Remove all influences which are linked to the given node and optimize the memory usage of the skinning info
         * afterwards.
         * @param nodeNr The node in the actor. All influences linked to this node will be removed from the skinning info.
         */
        void RemoveAllInfluencesForNode(uint32 nodeNr);

        /**
         * Collect all nodes to which the skinning info refers to.
         * @param influencedNodes The array to which the node numbers which are influenced by the skinning info will be added.
         * @param clearInfluencedNodesArray When set to true the given influenced nodes array will be cleared before filling it.
         */
        void CollectInfluencedNodes(MCore::Array<uint32>& influencedNodes, bool clearInfluencedNodesArray = true);

        /**
         * Optimize the skinning informations memory usage.
         * This shrinks the skinning influence data as much as possible.
         * This is automatically being called by the importer.
         */
        void OptimizeMemoryUsage();

        /**
         * Optimize the skinning influences & its memory usage.
         * This function assures that the output weights sum up to a value of 1.
         * @param tolerance The minimum weight value of an influence, smaller ones will be removed.
         * @param maxWeights The maximum number of influences per vertex you want to have in the output.
         */
        void OptimizeInfluences(float tolerance, uint32 maxWeights);

        /**
         * Reset the layer data to it's original data.
         * This is used to restore for example skinned data back into the data as it is in the base pose.
         * The mesh deformers will use this as a starting point then.
         */
        void ResetToOriginalData() override;

        /**
         * Collapse influences that use the same bones to just one influence.
         * If all influences of the attribute are linked to the same bone, it will collapse these
         * into just one influence with a weight of 1, which will have the same visual results.
         * The advantage of this is that CPU skinning will be much faster for those vertices, as just one influence has to be processed.
         * Please note that this function only works if all influences share the same bone. It does not perform any optimizations
         * when there are say 2 influences using bone A and another two influences using bone B. It only optimizes things when all influences are mapped to bone A.
         * @param attributeNr The attribute number (original vertex number) to apply this to.
         */
        void CollapseInfluences(uint32 attributeNr);

    private:
        MCore::Array2D<SkinInfluence>   mData;  /**< The stored influence data. The Array2D template allows a different number of skinning influences per vertex. */

        /**
         * The constructor.
         * @param numAttributes The number of attributes to store inside this layer.
         * @param allocData When set to true, it will already allocate space for the amount of attributes.
         */
        SkinningInfoVertexAttributeLayer(uint32 numAttributes, bool allocData = true);

        /**
         * The destructor, which should delete all allocated attributes from memory.
         */
        ~SkinningInfoVertexAttributeLayer();
    };
} // namespace EMotionFX
