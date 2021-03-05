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
#include "MeshBuilderVertexAttributeLayers.h"
#include <MCore/Source/Compare.h>
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MeshBuilderVertexLookup, MeshAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(MeshBuilderVertexAttributeLayer, MeshAllocator, 0)


    // creation
    MeshBuilderVertexAttributeLayerVector2* MeshBuilderVertexAttributeLayerVector2::Create(uint32 numOrgVerts, uint32 layerTypeID, bool isScale, bool isDeformable)
    {
        return new MeshBuilderVertexAttributeLayerVector2(numOrgVerts, layerTypeID, isScale, isDeformable);
    }

    MeshBuilderVertexAttributeLayerVector3* MeshBuilderVertexAttributeLayerVector3::Create(uint32 numOrgVerts, uint32 layerTypeID, bool isScale, bool isDeformable)
    {
        return new MeshBuilderVertexAttributeLayerVector3(numOrgVerts, layerTypeID, isScale, isDeformable);
    }

    MeshBuilderVertexAttributeLayerVector4* MeshBuilderVertexAttributeLayerVector4::Create(uint32 numOrgVerts, uint32 layerTypeID, bool isScale, bool isDeformable)
    {
        return new MeshBuilderVertexAttributeLayerVector4(numOrgVerts, layerTypeID, isScale, isDeformable);
    }

    MeshBuilderVertexAttributeLayerFloat* MeshBuilderVertexAttributeLayerFloat::Create(uint32 numOrgVerts, uint32 layerTypeID, bool isScale, bool isDeformable)
    {
        return new MeshBuilderVertexAttributeLayerFloat(numOrgVerts, layerTypeID, isScale, isDeformable);
    }

    MeshBuilderVertexAttributeLayerUInt32* MeshBuilderVertexAttributeLayerUInt32::Create(uint32 numOrgVerts, uint32 layerTypeID, bool isScale, bool isDeformable)
    {
        return new MeshBuilderVertexAttributeLayerUInt32(numOrgVerts, layerTypeID, isScale, isDeformable);
    }


    //------------------------------------------

    // endian conversions
    void MeshBuilderVertexAttributeLayerVector2::ConvertEndian(uint32 orgVtx, uint32 duplicate, MCore::Endian::EEndianType fromEndian, MCore::Endian::EEndianType targetEndian)
    {
        AZ::Vector2& value = mVertices[orgVtx][duplicate].mValue;
        MCore::Endian::ConvertVector2(&value, fromEndian, targetEndian);
    }

    void MeshBuilderVertexAttributeLayerVector3::ConvertEndian(uint32 orgVtx, uint32 duplicate, MCore::Endian::EEndianType fromEndian, MCore::Endian::EEndianType targetEndian)
    {
        AZ::Vector3& value = mVertices[orgVtx][duplicate].mValue;
        MCore::Endian::ConvertVector3(&value, fromEndian, targetEndian);
    }

    void MeshBuilderVertexAttributeLayerVector4::ConvertEndian(uint32 orgVtx, uint32 duplicate, MCore::Endian::EEndianType fromEndian, MCore::Endian::EEndianType targetEndian)
    {
        AZ::Vector4& value = mVertices[orgVtx][duplicate].mValue;
        MCore::Endian::ConvertVector4(&value, fromEndian, targetEndian);
    }

    void MeshBuilderVertexAttributeLayerFloat::ConvertEndian(uint32 orgVtx, uint32 duplicate, MCore::Endian::EEndianType fromEndian, MCore::Endian::EEndianType targetEndian)
    {
        float& value = mVertices[orgVtx][duplicate].mValue;
        MCore::Endian::ConvertFloat(&value, fromEndian, targetEndian);
    }

    void MeshBuilderVertexAttributeLayerUInt32::ConvertEndian(uint32 orgVtx, uint32 duplicate, MCore::Endian::EEndianType fromEndian, MCore::Endian::EEndianType targetEndian)
    {
        uint32& value = mVertices[orgVtx][duplicate].mValue;
        MCore::Endian::ConvertUnsignedInt32(&value, fromEndian, targetEndian);
    }


    //------------------------------------------

    // standard layer type compare functions
    bool MeshBuilderVertexAttributeLayerVector2::CheckIfIsVertexEqual(uint32 orgVtx, uint32 duplicate) const
    {
        const AZ::Vector2& value = mVertices[orgVtx][duplicate].mValue;
        return  (MCore::Compare<float>::CheckIfIsClose(value.GetX(), mVertexValue.GetX(), 0.00001f) &&
                 MCore::Compare<float>::CheckIfIsClose(value.GetY(), mVertexValue.GetY(), 0.00001f));
    }

    bool MeshBuilderVertexAttributeLayerVector3::CheckIfIsVertexEqual(uint32 orgVtx, uint32 duplicate) const
    {
        const AZ::Vector3& value = mVertices[orgVtx][duplicate].mValue;
        return  (
            MCore::Compare<float>::CheckIfIsClose(value.GetX(), mVertexValue.GetX(), 0.00001f) &&
            MCore::Compare<float>::CheckIfIsClose(value.GetY(), mVertexValue.GetY(), 0.00001f) &&
            MCore::Compare<float>::CheckIfIsClose(value.GetZ(), mVertexValue.GetZ(), 0.00001f));
    }

    bool MeshBuilderVertexAttributeLayerVector4::CheckIfIsVertexEqual(uint32 orgVtx, uint32 duplicate) const
    {
        const AZ::Vector4& value = mVertices[orgVtx][duplicate].mValue;
        return  (MCore::Compare<float>::CheckIfIsClose(value.GetX(), mVertexValue.GetX(), 0.00001f) &&
                 MCore::Compare<float>::CheckIfIsClose(value.GetY(), mVertexValue.GetY(), 0.00001f) &&
                 MCore::Compare<float>::CheckIfIsClose(value.GetZ(), mVertexValue.GetZ(), 0.00001f) &&
                 MCore::Compare<float>::CheckIfIsClose(value.GetW(), mVertexValue.GetW(), 0.00001f));
    }

    bool MeshBuilderVertexAttributeLayerFloat::CheckIfIsVertexEqual(uint32 orgVtx, uint32 duplicate) const
    {
        return MCore::Compare<float>::CheckIfIsClose(mVertices[orgVtx][duplicate].mValue, mVertexValue, 0.00001f);
    }

    bool MeshBuilderVertexAttributeLayerUInt32::CheckIfIsVertexEqual(uint32 orgVtx, uint32 duplicate) const
    {
        return (mVertices[orgVtx][duplicate].mValue == mVertexValue);
    }
}   // namespace EMotionFX
