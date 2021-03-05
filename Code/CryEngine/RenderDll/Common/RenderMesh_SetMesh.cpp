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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "RenderDll_precompiled.h"
#include "RenderMesh.h"
#include "IIndexedMesh.h"

#define TRANSFER_ALIGNMENT  1

static void transfer_writecombined(void* pDst, const void* pSrc, size_t size)
{
    cryMemcpy(pDst, pSrc, size, MC_CPU_TO_GPU);
}

template<EStreamIDs stream, size_t Size>
struct StreamCompactor;

template<EStreamIDs stream, size_t Size>
uint32 CompactStream(uint8 (&buffer)[Size], SSetMeshIntData& data, CMesh& mesh, uint32 beg, uint32 end)
{
    return StreamCompactor<stream, Size>::Compact(buffer, data, mesh, beg, end);
}

// Override for the VSF_GENERAL stream that can be used with arbitrary vertex data
template<EStreamIDs stream, size_t Size>
uint32 CompactStream(uint8(&buffer)[Size], SSetMeshIntData& data, CMesh& mesh, uint32 beg, uint32 end, const AZ::Vertex::Format& vertexFormat)
{
    return StreamCompactor<stream, Size>::Compact(buffer, data, mesh, beg, end, vertexFormat);
}

template<size_t Size>
struct StreamCompactor<VSF_GENERAL, Size>
{
    static void CompactPositions(uint8* stagingBuffer, [[maybe_unused]] SSetMeshIntData& data, CMesh& mesh, uint32 beg, uint32 end, uint32 streamIndex, uint32 vertexStride, uint32 attributeOffset, uint32 attributeByteLength)
    {
        if (mesh.Has32BitPositions())
        {
            Vec3* positions = mesh.GetStreamPtr<Vec3>(CMesh::POSITIONS, streamIndex);
            AZ_Assert(attributeByteLength == sizeof(Vec3), "Mesh uses three 32 bit floats for positions, but vertex format is expecting a different size");
            for (size_t i = 0; i < end; ++i)
            {
                memcpy(stagingBuffer + i * vertexStride + attributeOffset, &positions[beg + i], attributeByteLength);
            }
        }
        else if (mesh.Has16BitPositions())
        {
            Vec3f16* positions = mesh.GetStreamPtr<Vec3f16>(CMesh::POSITIONSF16, streamIndex);
            AZ_Assert(attributeByteLength == sizeof(Vec3f16), "Mesh uses three 16 bit floats for positions, but vertex format is expecting a different size");
            for (size_t i = 0; i < end; ++i)
            {
                memcpy(stagingBuffer + i * vertexStride + attributeOffset, &positions[beg + i], attributeByteLength);
            }
        }
        else
        {
            AZ_Assert(false, "Mesh has no per-vertex positions.");
        }
    }

    static void CompactNormals(SVF_P3S_N4B_C4B_T2S* pVBuff, SSetMeshIntData& data, CMesh& mesh, uint32 beg, uint32 end)
    {
        SMeshNormal* normals = mesh.GetStreamPtr<SMeshNormal>(CMesh::NORMALS);
        if (normals)
        {
            for (size_t i = 0; i < end; ++i)
            {
                Vec3 n = normals[beg + i].GetN();

                pVBuff[i].normal.bcolor[0] = (byte)(n[0] * 127.5f + 128.0f);
                pVBuff[i].normal.bcolor[1] = (byte)(n[1] * 127.5f + 128.0f);
                pVBuff[i].normal.bcolor[2] = (byte)(n[2] * 127.5f + 128.0f);

                SwapEndian(pVBuff[i].normal.dcolor);
            }
        }
    }

    static void CompactColors(uint8* stagingBuffer, [[maybe_unused]] SSetMeshIntData& data, CMesh& mesh, uint32 beg, uint32 end, uint32 streamIndex, uint32 vertexStride, uint32 attributeOffset, uint32 attributeByteLength)
    {
        SMeshColor* colors = mesh.GetStreamPtr<SMeshColor>(CMesh::COLORS, streamIndex);
        if (colors)
        {
            for (size_t i = 0; i < end; ++i)
            {
                ColorB color = colors[beg + i].GetRGBA();
                UCol uColor;
                uColor.bcolor[0] = color.b;
                uColor.bcolor[1] = color.g;
                uColor.bcolor[2] = color.r;
                uColor.bcolor[3] = color.a;
                SwapEndian(uColor.dcolor);
                memcpy(stagingBuffer + i * vertexStride + attributeOffset, &uColor, attributeByteLength);
            }
        }
        else
        {
            for (size_t i = 0; i < end; ++i)
            {
                UCol defaultColor;
                defaultColor.dcolor = ~0;
                memcpy(stagingBuffer + i * vertexStride + attributeOffset, &defaultColor, attributeByteLength);
            }
        }
    }

    static void CompactUVs(uint8* stagingBuffer, [[maybe_unused]] SSetMeshIntData& data, CMesh& mesh, uint32 beg, uint32 end, uint32 streamIndex, uint32 vertexStride, uint32 attributeOffset, uint32 attributeByteLength)
    {
        SMeshTexCoord* texCoords = mesh.GetStreamPtr<SMeshTexCoord>(CMesh::TEXCOORDS, streamIndex);
        if (texCoords)
        {
            for (size_t i = 0; i < end; ++i)
            {
                SMeshTexCoord meshTexCoord = texCoords[beg + i];
                // If the vertex format uses two 32 bit floats for texture coordinates, copy them to the staging buffer
                if (attributeByteLength == sizeof(Vec2))
                {
                    Vec2 texCoord = meshTexCoord.GetUV();
                    memcpy(stagingBuffer + i * vertexStride + attributeOffset, &texCoord, attributeByteLength);
                }
                // If the vertex format uses two 16 bit floats for texture coordinates, convert the CMesh texture coordinates to 16 bit floats and then copy them to the staging buffer
                else if (attributeByteLength == sizeof(Vec2f16))
                {
                    Vec2f16 texCoord;
                    meshTexCoord.ExportTo(texCoord);
                    memcpy(stagingBuffer + i * vertexStride + attributeOffset, &texCoord, attributeByteLength);
                }
                else
                {
                    AZ_Assert(false, "Invalid byte length for texture coordinates");
                }
            }
        }
    }

    static uint32 Compact(uint8 (&buffer)[Size], SSetMeshIntData& data, CMesh& mesh, uint32 beg, uint32 end, const AZ::Vertex::Format& vertexFormat)
    {
        if (data.m_pVBuff)
        {

            size_t dstPad = (size_t)&data.m_pVBuff[beg * vertexFormat.GetStride()] & (TRANSFER_ALIGNMENT - 1);
            uint8* stagingBuffer = &buffer[dstPad];
            uint32 amount = min((uint32)(end - beg), (uint32)(Size / vertexFormat.GetStride()));

            if (mesh.m_pP3S_C4B_T2S)
            {
                // Do a straight copy of mesh data that has already been interleaved
                memcpy(stagingBuffer, &mesh.m_pP3S_C4B_T2S[beg], sizeof(SVF_P3S_C4B_T2S) * amount);
            }
            else
            {
                uint32 attributeCount = 0;
                const uint8* attributes = vertexFormat.GetAttributes(attributeCount);
                int attributeCounter[(int)AZ::Vertex::AttributeUsage::NumUsages] = { 0 };
                uint32 attributeOffset = 0;
                // Iterate over each attribute in the vertex format and interleave that attribute into the staging buffer

                for (uint ii = 0; ii < attributeCount; ++ii)
                {
                    const uint8 attribute = attributes[ii];
                    const AZ::Vertex::AttributeUsage usage = AZ::Vertex::Attribute::GetUsage(attribute);
                    switch (usage)
                    {
                    case AZ::Vertex::AttributeUsage::Position:
                        CompactPositions(stagingBuffer, data, mesh, beg, amount, attributeCounter[(int)AZ::Vertex::AttributeUsage::Position], vertexFormat.GetStride(), attributeOffset, AZ::Vertex::Attribute::GetByteLength(attribute));
                        break;
                    case AZ::Vertex::AttributeUsage::Color:
                        CompactColors(stagingBuffer, data, mesh, beg, amount, attributeCounter[(int)AZ::Vertex::AttributeUsage::Color], vertexFormat.GetStride(), attributeOffset, AZ::Vertex::Attribute::GetByteLength(attribute));
                        break;
                    case AZ::Vertex::AttributeUsage::TexCoord:
                        CompactUVs(stagingBuffer, data, mesh, beg, amount, attributeCounter[(int)AZ::Vertex::AttributeUsage::TexCoord], vertexFormat.GetStride(), attributeOffset, AZ::Vertex::Attribute::GetByteLength(attribute));
                        break;
                    default:
                        AZ_Assert(false, "No case to handle per vertex data in the VSF_GENERAL stream for usage %d.", AZ::Vertex::Attribute::GetUsage(attribute));
                        break;
                    }
                    // Keep track of the offset of the current attribute
                    attributeOffset += AZ::Vertex::Attribute::GetByteLength(attribute);
                    // Keep track of the number of attributes with a given usage so we can use that to index into the CMesh's vertex streams for that usage
                    attributeCounter[static_cast<uint8>(usage)]++;
                }
            }
            transfer_writecombined(&data.m_pVBuff[beg * vertexFormat.GetStride()], &buffer[dstPad], amount * vertexFormat.GetStride());
            return amount;
        }

        CryFatalError("CRenderMesh::SetMesh_Impl: invalid vertex format for general stream");
        return 0;
    }
};

template<size_t Size>
struct StreamCompactor<VSF_TANGENTS, Size>
{
    static uint32 Compact(uint8 (&buffer)[Size], SSetMeshIntData& data, CMesh& mesh, uint32 beg, uint32 end)
    {
        if (mesh.m_pTangents == NULL || data.m_pTBuff == NULL)
        {
            return end;
        }

        uint32 dstPad = 0;

        SPipTangents* pTBuff = alias_cast<SPipTangents*>(&buffer[dstPad]);
        uint32 amount = min((uint32)(end - beg), (uint32)(Size / sizeof(pTBuff[0])));

        for (size_t i = 0; i < amount; ++i)
        {
            mesh.m_pTangents[beg + i].ExportTo(pTBuff[i]);
        }

        transfer_writecombined(&data.m_pTBuff[beg], &buffer[dstPad], amount * sizeof(data.m_pTBuff[0]));

        return amount;
    }
};

template<size_t Size>
struct StreamCompactor<VSF_QTANGENTS, Size>
{
    static uint32 Compact(uint8 (&buffer)[Size], SSetMeshIntData& data, CMesh& mesh, uint32 beg, uint32 end)
    {
        if (mesh.m_pQTangents == NULL || data.m_pQTBuff == NULL)
        {
            return end;
        }

        uint32 dstPad = 0;

        SPipQTangents* pQTBuff = alias_cast<SPipQTangents*>(&buffer[dstPad]);
        uint32 amount = min((uint32)(end - beg), (uint32)(Size / sizeof(pQTBuff[0])));

        for (size_t i = 0; i < amount; ++i)
        {
            mesh.m_pQTangents[beg + i].ExportTo(pQTBuff[i]);
        }

        transfer_writecombined(&data.m_pQTBuff[beg], &buffer[dstPad], amount * sizeof(pQTBuff[0]));

        return amount;
    }
};

#if ENABLE_NORMALSTREAM_SUPPORT
template<size_t Size>
struct StreamCompactor<VSF_NORMALS, Size>
{
    static uint32 Compact(uint8 (&buffer)[Size], SSetMeshIntData& data, CMesh& mesh, uint32 beg, uint32 end)
    {
        if (mesh.m_pNorms == NULL || data.m_pNormalsBuff == NULL)
        {
            return end;
        }

        uint32 dstPad = 0;

        uint32 amount = min((uint32)(end - beg), (uint32)(Size / sizeof(data.m_pNormalsBuff[0])));
        memcpy(&buffer[dstPad], &mesh.m_pNorms[beg], amount * sizeof(data.m_pNormalsBuff[0]));
        transfer_writecombined(&data.m_pNormalsBuff[beg], &buffer[dstPad], amount * sizeof(data.m_pNormalsBuff[0]));

        return amount;
    }
};
#endif

template<size_t Size>
struct StreamCompactor<VSF_VERTEX_VELOCITY, Size>
{
    static uint32 Compact(uint8 (&buffer)[Size], SSetMeshIntData& data, [[maybe_unused]] CMesh& mesh, uint32 beg, uint32 end)
    {
        if (data.m_pVelocities == NULL)
        {
            return end;
        }

        uint32 dstPad = 0;

        uint32 amount = min((uint32)(end - beg), (uint32)(Size / sizeof(data.m_pVelocities[0])));
        memset(&buffer[dstPad], 0x0, amount * sizeof(data.m_pVelocities[0]));
        transfer_writecombined(&data.m_pVelocities[beg], &buffer[dstPad], amount * sizeof(data.m_pVelocities[0]));

        return amount;
    }
};


template<size_t Size>
uint32 CompactIndices(uint8 (&buffer)[Size], SSetMeshIntData& data, CMesh& mesh, uint32 beg, uint32 end)
{
    if (mesh.m_pIndices == NULL || data.m_pInds == NULL)
    {
        return end;
    }

    uint32 dstPad = 0;

    uint32 amount = min((uint32)(end - beg), (uint32)(Size / sizeof(mesh.m_pIndices[0])));
    memcpy(&buffer[dstPad], &mesh.m_pIndices[beg], amount * sizeof(mesh.m_pIndices[0]));
    transfer_writecombined(&data.m_pInds[beg], &buffer[dstPad], amount * sizeof(data.m_pInds[0]));
    return amount;
}

typedef _MS_ALIGN (128) uint8 AlignedStagingBufferT[(8 << 10) + 128];

void CRenderMesh::SetMesh_IntImpl(SSetMeshIntData data)
{
    CMesh& mesh = *data.m_pMesh;
    AlignedStagingBufferT stagingBuffer;

    //////////////////////////////////////////////////////////////////////////
    // Compact the seperate streams from the CMesh instance into a general
    //////////////////////////////////////////////////////////////////////////
    for (uint32 iter = 0; iter < data.m_nVerts; iter += CompactStream<VSF_GENERAL>(stagingBuffer, data, mesh, iter, data.m_nVerts, m_vertexFormat))
    {
        ;
    }
    for (uint32 iter = 0; iter < data.m_nVerts; iter += CompactStream<VSF_TANGENTS>(stagingBuffer, data, mesh, iter, data.m_nVerts))
    {
        ;
    }
    for (uint32 iter = 0; iter < data.m_nVerts; iter += CompactStream<VSF_QTANGENTS>(stagingBuffer, data, mesh, iter, data.m_nVerts))
    {
        ;
    }
# if ENABLE_NORMALSTREAM_SUPPORT
    for (uint32 iter = 0; iter < data.m_nVerts; iter += CompactStream<VSF_NORMALS>(stagingBuffer, data, mesh, iter, data.m_nVerts))
    {
        ;
    }
# endif
    for (uint32 iter = 0; iter < data.m_nVerts; iter += CompactStream<VSF_VERTEX_VELOCITY>(stagingBuffer, data, mesh, iter, data.m_nVerts))
    {
        ;
    }
    for (uint32 iter = 0; iter < data.m_nInds; iter += CompactIndices(stagingBuffer, data, mesh, iter, data.m_nInds))
    {
        ;
    }
}
