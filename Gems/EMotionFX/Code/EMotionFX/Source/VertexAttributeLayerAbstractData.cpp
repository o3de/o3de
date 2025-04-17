/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "VertexAttributeLayerAbstractData.h"
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(VertexAttributeLayerAbstractData, MeshAllocator)


    // constructor
    VertexAttributeLayerAbstractData::VertexAttributeLayerAbstractData(uint32 numAttributes, uint32 typeID, uint32 attribSizeInBytes, bool keepOriginals)
        : VertexAttributeLayer(numAttributes, keepOriginals)
    {
        m_data               = nullptr;
        m_swapBuffer         = nullptr;
        m_typeId             = typeID;
        m_attribSizeInBytes  = attribSizeInBytes;

        // allocate the data
        const uint32 numBytes = CalcTotalDataSizeInBytes(m_keepOriginals);
        m_data = (uint8*)MCore::AlignedAllocate(numBytes, 16, EMFX_MEMCATEGORY_GEOMETRY_VERTEXATTRIBUTES);
    }


    // the destructor
    VertexAttributeLayerAbstractData::~VertexAttributeLayerAbstractData()
    {
        MCore::AlignedFree(m_data);
        MCore::AlignedFree(m_swapBuffer);
    }


    // create
    VertexAttributeLayerAbstractData* VertexAttributeLayerAbstractData::Create(uint32 numAttributes, uint32 typeID, uint32 attribSizeInBytes, bool keepOriginals)
    {
        return aznew VertexAttributeLayerAbstractData(numAttributes, typeID, attribSizeInBytes, keepOriginals);
    }


    // get the layer type
    uint32 VertexAttributeLayerAbstractData::GetType() const
    {
        return m_typeId;
    }


    // get the type string
    const char* VertexAttributeLayerAbstractData::GetTypeString() const
    {
        return "VertexAttributeLayerAbstractData";
    }


    // calculate the total data size
    uint32 VertexAttributeLayerAbstractData::CalcTotalDataSizeInBytes(bool includeOriginals) const
    {
        if (includeOriginals && m_keepOriginals)
        {
            return (m_attribSizeInBytes * m_numAttributes) << 1; // multiplied by two, as we store the originals right after it
        }
        else
        {
            return m_attribSizeInBytes * m_numAttributes;
        }
    }


    // clone the layer
    VertexAttributeLayer* VertexAttributeLayerAbstractData::Clone()
    {
        // create the clone
        VertexAttributeLayerAbstractData* clone = aznew VertexAttributeLayerAbstractData(m_numAttributes, m_typeId, m_attribSizeInBytes, m_keepOriginals);

        // copy over the data
        uint8* cloneData = (uint8*)clone->GetData();
        MCore::MemCopy(cloneData, m_data, CalcTotalDataSizeInBytes(true));

        clone->m_nameId = m_nameId;
        return clone;
    }


    // reset current data into original data, if present
    void VertexAttributeLayerAbstractData::ResetToOriginalData()
    {
        // if we dont have any original data, there is nothing to do
        if (m_keepOriginals == false)
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
        if (m_swapBuffer == nullptr)
        {
            m_swapBuffer = (uint8*)MCore::AlignedAllocate(m_attribSizeInBytes, 16, EMFX_MEMCATEGORY_GEOMETRY_VERTEXATTRIBUTES);
        }

        // get the locations of where the attributes are stored
        uint8* attribPtrA = (uint8*)GetData(attribA);
        uint8* attribPtrB = (uint8*)GetData(attribB);

        // swap the attribute data
        MCore::MemCopy(m_swapBuffer, attribPtrA, m_attribSizeInBytes);    // copy attribute A into the temp swap buffer
        MCore::MemCopy(attribPtrA, attribPtrB, m_attribSizeInBytes);     // copy attribute B into A
        MCore::MemCopy(attribPtrB, m_swapBuffer, m_attribSizeInBytes);    // copy the temp swap buffer data into attribute B

        // swap the originals
        if (m_keepOriginals)
        {
            attribPtrA = (uint8*)GetOriginalData(attribA);
            attribPtrB = (uint8*)GetOriginalData(attribB);
            MCore::MemCopy(m_swapBuffer, attribPtrA, m_attribSizeInBytes);    // copy attribute A into the temp swap buffer
            MCore::MemCopy(attribPtrA, attribPtrB, m_attribSizeInBytes);     // copy attribute B into A
            MCore::MemCopy(attribPtrB, m_swapBuffer, m_attribSizeInBytes);    // copy the temp swap buffer data into attribute B
        }
    }


    // remove the swap buffer from memory
    void VertexAttributeLayerAbstractData::RemoveSwapBuffer()
    {
        MCore::AlignedFree(m_swapBuffer);
        m_swapBuffer = nullptr;
    }


    // remove a given set of attributes
    void VertexAttributeLayerAbstractData::RemoveAttributes(uint32 startAttributeNr, uint32 endAttributeNr)
    {
        // perform some checks on the input data
        MCORE_ASSERT(startAttributeNr < m_numAttributes);
        MCORE_ASSERT(endAttributeNr < m_numAttributes);

        // Store the original number of bytes for the reallocation
        const size_t numOriginalBytes = CalcTotalDataSizeInBytes(m_keepOriginals);

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
        const uint32 numBytesToMove = (m_numAttributes - end - 1) * m_attribSizeInBytes;
        if (numBytesToMove > 0)
        {
            MCore::MemMove(m_data + start * m_attribSizeInBytes, m_data + (end + 1) * m_attribSizeInBytes, numBytesToMove);
        }

        // remove the attributes from the original data
        if (m_keepOriginals)
        {
            // remove them from the original data
            uint8* orgData = (uint8*)GetOriginalData();

            if (numBytesToMove > 0)
            {
                MCore::MemMove(orgData + start * m_attribSizeInBytes, orgData + (end + 1) * m_attribSizeInBytes, numBytesToMove);
            }

            // remove the created gap between the current data and original data, as both original and current data remain in the same continuous piece of memory
            MCore::MemMove(m_data + (m_numAttributes - numAttribsToRemove) * m_attribSizeInBytes, orgData, (m_numAttributes - numAttribsToRemove) * m_attribSizeInBytes);
        }

        // decrease the number of attributes
        m_numAttributes -= numAttribsToRemove;

        // reallocate, to make the data array smaller
        const uint32 numBytes = CalcTotalDataSizeInBytes(m_keepOriginals);
        m_data = (uint8*)MCore::AlignedRealloc(m_data, numBytes, numOriginalBytes, 16, EMFX_MEMCATEGORY_GEOMETRY_VERTEXATTRIBUTES);
    }


    // yes, this is the abstract data class
    bool VertexAttributeLayerAbstractData::GetIsAbstractDataClass() const
    {
        return true;
    }
}   // namespace EMotionFX
