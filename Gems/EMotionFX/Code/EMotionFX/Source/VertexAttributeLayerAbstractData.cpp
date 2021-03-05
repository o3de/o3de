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

// include the required headers
#include "VertexAttributeLayerAbstractData.h"
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(VertexAttributeLayerAbstractData, MeshAllocator, 0)


    // constructor
    VertexAttributeLayerAbstractData::VertexAttributeLayerAbstractData(uint32 numAttributes, uint32 typeID, uint32 attribSizeInBytes, bool keepOriginals)
        : VertexAttributeLayer(numAttributes, keepOriginals)
    {
        mData               = nullptr;
        mSwapBuffer         = nullptr;
        mTypeID             = typeID;
        mAttribSizeInBytes  = attribSizeInBytes;

        // allocate the data
        const uint32 numBytes = CalcTotalDataSizeInBytes(mKeepOriginals);
        mData = (uint8*)MCore::AlignedAllocate(numBytes, 16, EMFX_MEMCATEGORY_GEOMETRY_VERTEXATTRIBUTES);
    }


    // the destructor
    VertexAttributeLayerAbstractData::~VertexAttributeLayerAbstractData()
    {
        MCore::AlignedFree(mData);
        MCore::AlignedFree(mSwapBuffer);
    }


    // create
    VertexAttributeLayerAbstractData* VertexAttributeLayerAbstractData::Create(uint32 numAttributes, uint32 typeID, uint32 attribSizeInBytes, bool keepOriginals)
    {
        return aznew VertexAttributeLayerAbstractData(numAttributes, typeID, attribSizeInBytes, keepOriginals);
    }


    // get the layer type
    uint32 VertexAttributeLayerAbstractData::GetType() const
    {
        return mTypeID;
    }


    // get the type string
    const char* VertexAttributeLayerAbstractData::GetTypeString() const
    {
        return "VertexAttributeLayerAbstractData";
    }


    // calculate the total data size
    uint32 VertexAttributeLayerAbstractData::CalcTotalDataSizeInBytes(bool includeOriginals) const
    {
        if (includeOriginals && mKeepOriginals)
        {
            return (mAttribSizeInBytes * mNumAttributes) << 1; // multiplied by two, as we store the originals right after it
        }
        else
        {
            return mAttribSizeInBytes * mNumAttributes;
        }
    }


    // clone the layer
    VertexAttributeLayer* VertexAttributeLayerAbstractData::Clone()
    {
        // create the clone
        VertexAttributeLayerAbstractData* clone = aznew VertexAttributeLayerAbstractData(mNumAttributes, mTypeID, mAttribSizeInBytes, mKeepOriginals);

        // copy over the data
        uint8* cloneData = (uint8*)clone->GetData();
        MCore::MemCopy(cloneData, mData, CalcTotalDataSizeInBytes(true));

        clone->mNameID = mNameID;
        return clone;
    }


    // reset current data into original data, if present
    void VertexAttributeLayerAbstractData::ResetToOriginalData()
    {
        // if we dont have any original data, there is nothing to do
        if (mKeepOriginals == false)
        {
            return;
        }

        // get the data pointers
        uint8* originalData = (uint8*)GetOriginalData();
        uint8* data         = (uint8*)GetData();

        // copy the original data over the current data
        MCore::MemCopy(data, originalData, CalcTotalDataSizeInBytes(false));
    }


    // swap two attributes
    void VertexAttributeLayerAbstractData::SwapAttributes(uint32 attribA, uint32 attribB)
    {
        // create a swap buffer if we haven't got it already
        if (mSwapBuffer == nullptr)
        {
            mSwapBuffer = (uint8*)MCore::AlignedAllocate(mAttribSizeInBytes, 16, EMFX_MEMCATEGORY_GEOMETRY_VERTEXATTRIBUTES);
        }

        // get the locations of where the attributes are stored
        uint8* attribPtrA = (uint8*)GetData(attribA);
        uint8* attribPtrB = (uint8*)GetData(attribB);

        // swap the attribute data
        MCore::MemCopy(mSwapBuffer, attribPtrA, mAttribSizeInBytes);    // copy attribute A into the temp swap buffer
        MCore::MemCopy(attribPtrA, attribPtrB, mAttribSizeInBytes);     // copy attribute B into A
        MCore::MemCopy(attribPtrB, mSwapBuffer, mAttribSizeInBytes);    // copy the temp swap buffer data into attribute B

        // swap the originals
        if (mKeepOriginals)
        {
            attribPtrA = (uint8*)GetOriginalData(attribA);
            attribPtrB = (uint8*)GetOriginalData(attribB);
            MCore::MemCopy(mSwapBuffer, attribPtrA, mAttribSizeInBytes);    // copy attribute A into the temp swap buffer
            MCore::MemCopy(attribPtrA, attribPtrB, mAttribSizeInBytes);     // copy attribute B into A
            MCore::MemCopy(attribPtrB, mSwapBuffer, mAttribSizeInBytes);    // copy the temp swap buffer data into attribute B
        }
    }


    // remove the swap buffer from memory
    void VertexAttributeLayerAbstractData::RemoveSwapBuffer()
    {
        MCore::AlignedFree(mSwapBuffer);
        mSwapBuffer = nullptr;
    }


    // remove a given set of attributes
    void VertexAttributeLayerAbstractData::RemoveAttributes(uint32 startAttributeNr, uint32 endAttributeNr)
    {
        // perform some checks on the input data
        MCORE_ASSERT(startAttributeNr < mNumAttributes);
        MCORE_ASSERT(endAttributeNr < mNumAttributes);

        // Store the original number of bytes for the reallocation
        const size_t numOriginalBytes = CalcTotalDataSizeInBytes(mKeepOriginals);

        // make sure the start attribute number is lower than the end
        uint32 start = startAttributeNr;
        uint32 end   = endAttributeNr;
        if (start > end)
        {
            uint32 temp = start;
            start = end;
            end = temp;
        }

        //------------
        // if there isn't anything to remove, quit
        const uint32 numAttribsToRemove = (end - start) + 1; // +1 because we remove the last one as well
        if (numAttribsToRemove == 0)
        {
            return;
        }

        // remove the attributes from the current data
        const uint32 numBytesToMove = (mNumAttributes - end - 1) * mAttribSizeInBytes;
        if (numBytesToMove > 0)
        {
            MCore::MemMove(mData + start * mAttribSizeInBytes, mData + (end + 1) * mAttribSizeInBytes, numBytesToMove);
        }

        // remove the attributes from the original data
        if (mKeepOriginals)
        {
            // remove them from the original data
            uint8* orgData = (uint8*)GetOriginalData();

            if (numBytesToMove > 0)
            {
                MCore::MemMove(orgData + start * mAttribSizeInBytes, orgData + (end + 1) * mAttribSizeInBytes, numBytesToMove);
            }

            // remove the created gap between the current data and original data, as both original and current data remain in the same continuous piece of memory
            MCore::MemMove(mData + (mNumAttributes - numAttribsToRemove) * mAttribSizeInBytes, orgData, (mNumAttributes - numAttribsToRemove) * mAttribSizeInBytes);
        }

        // decrease the number of attributes
        mNumAttributes -= numAttribsToRemove;

        // reallocate, to make the data array smaller
        const uint32 numBytes = CalcTotalDataSizeInBytes(mKeepOriginals);
        mData = (uint8*)MCore::AlignedRealloc(mData, numBytes, numOriginalBytes, 16, EMFX_MEMCATEGORY_GEOMETRY_VERTEXATTRIBUTES);
    }


    // yes, this is the abstract data class
    bool VertexAttributeLayerAbstractData::GetIsAbstractDataClass() const
    {
        return true;
    }
}   // namespace EMotionFX
