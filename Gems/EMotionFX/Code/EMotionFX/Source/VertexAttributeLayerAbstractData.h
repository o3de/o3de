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


namespace EMotionFX
{
    // forward declarations
    class Node;
    class Mesh;
    class Actor;


    /**
     * The abstract data vertex attribute layer base class.
     * This abstract data class can hold any type of attribute that has a fixed size per vertex.
     */
    class EMFX_API VertexAttributeLayerAbstractData
        : public VertexAttributeLayer
    {
        AZ_CLASS_ALLOCATOR_DECL

    public:
        /**
         * The creation method.
         * @param numAttributes The number of attributes to store inside this layer.
         * @param typeID The type ID of the data. This could be something that identifies position data, normals, or anything else. So not the data type itself, but what
         *               it will be used for. Some symantic value.
         * @param attribSizeInBytes The size, in bytes, of a single attribute. For example if the data stored internally will be a Vector3, this would be sizeof(Vector3), which would be 12 bytes.
         * @param keepOriginals Specifies whether a copy of the original data should be stored as well. The current data values wil then be restored
         *                      to their original values every frame, before passing them through any mesh deformers.
         */
        static VertexAttributeLayerAbstractData* Create(uint32 numAttributes, uint32 typeID, uint32 attribSizeInBytes, bool keepOriginals = false);

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
         * Clone the vertex attribute layer.
         * @result A clone of this layer.
         */
        VertexAttributeLayer* Clone() override;

        /**
         * Reset the layer data to it's original data.
         * This is used to restore for example skinned data back into the data as it is in the base pose.
         * The mesh deformers will use this as a starting point then.
         */
        void ResetToOriginalData() override;

        /**
         * Swap the data for two attributes.
         * You specify two attribute numbers, the data for them should be swapped.
         * This is used by the geometry LOD system and will be called by Mesh::SwapVertex as well.
         * @param attribA The first attribute number.
         * @param attribB The second attribute number.
         */
        void SwapAttributes(uint32 attribA, uint32 attribB) override;

        /**
         * Remove a range of attributes.
         * @param startAttributeNr The start attribute number.
         * @param endAttributeNr The end attribute number, which will also be removed.
         */
        void RemoveAttributes(uint32 startAttributeNr, uint32 endAttributeNr) override;

        /**
         * Get a pointer to the data for a given attribute. You have to typecast the data yourself.
         * @result A pointer to the vertex data of the specified attribute number.
         */
        MCORE_INLINE void* GetData(uint32 attributeNr)                  { return (mData + mAttribSizeInBytes * attributeNr);  }

        /**
         * Get a pointer to the data. You have to typecast the data yourself.
         * @result A pointer to the vertex data.
         */
        MCORE_INLINE void* GetData() override                           { return mData; }

        /**
         * Get the size of one attribute in bytes.
         * So if you create a layer that stores Vector3 objects, the size returned should be
         * equal to sizeof(Vector3).
         * @result The size of a single attribute, in bytes.
         */
        MCORE_INLINE uint32 GetAttributeSizeInBytes() const             { return mAttribSizeInBytes; }

        /**
         * Get a pointer to the original data, as it is stored in the base pose, before any mesh deformers have been applied.
         * If there is no original data, it will just return a pointer to the regular data.
         * @result A pointer to the original vertex data, or the data as returned by GetData() if there isn't any original data.
         */
        MCORE_INLINE void* GetOriginalData() override
        {
            if (mKeepOriginals)
            {
                return (mData + (mAttribSizeInBytes * mNumAttributes));
            }
            else
            {
                return mData;
            }
        }

        /**
         * Get a pointer to the original data for a given attribute, as it is stored in the base pose, before any mesh deformers have been applied.
         * If there is no original data, it will just return a pointer to the regular data for the given attribute.
         * @result A pointer to the original vertex data, or the data as returned by GetData(attributeNr) if there isn't any original data.
         */
        MCORE_INLINE void* GetOriginalData(uint32 attributeNr)
        {
            if (mKeepOriginals)
            {
                return (mData + (mAttribSizeInBytes * mNumAttributes) + (mAttribSizeInBytes * attributeNr));
            }
            else
            {
                return (mData + mAttribSizeInBytes * attributeNr);
            }
        }

        /**
         * Calculate and return the total size of the layer data, in bytes.
         * This multiplies the number of attributes with the size of one single attribute, in bytes.
         * Optionally you can also choose to include the original data.
         * @param includeOriginals Set to true if you want to include the original vertex data as well.
         * @result Returns the size, in bytes, of the total attribute data that is being stored.
         */
        uint32 CalcTotalDataSizeInBytes(bool includeOriginals) const;

        /**
         * Remove a temp swap buffer from memory.
         * This temp swap buffer is a pre-allocated buffer, which will prevent having to do an allocation every time
         * you call the SwapAttributes method, which is for example used by the LOD generator of EMotion FX.
         */
        void RemoveSwapBuffer();

        /**
         * Returns true when this is the VertexAttributeLayerAbstractData class.
         * It is just an internal thing that was needed to make the whole abstract data idea work.
         * @result Returns always true for this class.
         */
        bool GetIsAbstractDataClass() const override;

    private:
        uint8*  mData;                  /**< The buffer containing the data. */
        uint8*  mSwapBuffer;            /**< The swap buffer, used for swapping items. This will only be allocated once you call SwapAttribute, like the LOD generation system does. */
        uint32  mAttribSizeInBytes;     /**< The size of a single attribute, in bytes. */
        uint32  mTypeID;                /**< The type ID that identifies the type of the data, for example if it is position data, normal data, or colors, etc. */

        /**
         * The constructor.
         * @param numAttributes The number of attributes to store inside this layer.
         * @param typeID The type ID of the data. This could be something that identifies position data, normals, or anything else. So not the data type itself, but what
         *               it will be used for. Some symantic value.
         * @param attribSizeInBytes The size, in bytes, of a single attribute. For example if the data stored internally will be a Vector3, this would be sizeof(Vector3), which would be 12 bytes.
         * @param keepOriginals Specifies whether a copy of the original data should be stored as well. The current data values wil then be restored
         *                      to their original values every frame, before passing them through any mesh deformers.
         */
        VertexAttributeLayerAbstractData(uint32 numAttributes, uint32 typeID, uint32 attribSizeInBytes, bool keepOriginals = false);

        /**
         * The destructor, which should delete all allocated attributes from memory.
         */
        ~VertexAttributeLayerAbstractData();
    };
} // namespace EMotionFX
