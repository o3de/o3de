/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Generation/Components/TangentGenerator/TangentGenerators/BlendShapeMikkTGenerator.h>
#include <Generation/Components/TangentGenerator/TangentGenerateComponent.h>

#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneData/GraphData/BlendShapeData.h>

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>

#include <mikkelsen/mikktspace.h>

namespace AZ::TangentGeneration::BlendShape::MikkT
{
    // Returns the number of triangles in the mesh.
    int GetNumFaces(const SMikkTSpaceContext* context)
    {
        MikktCustomData* customData = static_cast<MikktCustomData*>(context->m_pUserData);
        return customData->m_blendShapeData->GetFaceCount();
    }

    int GetNumVerticesOfFace([[maybe_unused]] const SMikkTSpaceContext* context, [[maybe_unused]] const int face)
    {
        return 3;
    }

    void GetPosition(const SMikkTSpaceContext* context, float posOut[], const int face, const int vert)
    {
        MikktCustomData* customData = static_cast<MikktCustomData*>(context->m_pUserData);
        const AZ::u32 vertexIndex = customData->m_blendShapeData->GetFaceVertexIndex(face, vert);
        const AZ::Vector3& pos = customData->m_blendShapeData->GetPosition(vertexIndex);
        posOut[0] = pos.GetX();
        posOut[1] = pos.GetY();
        posOut[2] = pos.GetZ();
    }

    void GetNormal(const SMikkTSpaceContext* context, float normOut[], const int face, const int vert)
    {
        MikktCustomData* customData = static_cast<MikktCustomData*>(context->m_pUserData);
        const AZ::u32 vertexIndex = customData->m_blendShapeData->GetFaceVertexIndex(face, vert);
        const AZ::Vector3 normal = customData->m_blendShapeData->GetNormal(vertexIndex).GetNormalizedSafe();
        normOut[0] = normal.GetX();
        normOut[1] = normal.GetY();
        normOut[2] = normal.GetZ();
    }

    void GetTexCoord(const SMikkTSpaceContext* context, float texOut[], const int face, const int vert)
    {
        MikktCustomData* customData = static_cast<MikktCustomData*>(context->m_pUserData);
        const AZ::u32 vertexIndex = customData->m_blendShapeData->GetFaceVertexIndex(face, vert);
        const AZ::Vector2& uv = customData->m_blendShapeData->GetUV(vertexIndex, static_cast<unsigned int>(customData->m_uvSetIndex));
        texOut[0] = uv.GetX();
        texOut[1] = uv.GetY();
    }

    // This function is used to return tangent space results to the application.
    // tangent and bitangent are unit length vectors and magS and magT are their
    // true magnitudes which can be used for relief mapping effects.
    // bitangent is the "real" bitangent and thus may not be perpendicular to tangent.
    // However, both are perpendicular to the vertex normal.
    // For normal maps it is sufficient to use the following simplified version of the bitangent which is generated at pixel/vertex level.
    // signValue = isOrientationPreserving ? 1.0f : -1.0f;
    // bitangent = signValue * cross(vN, tangent);
    void SetTSpace(const SMikkTSpaceContext* context, const float tangent[], const float bitangent[], const float magS, const float magT, const tbool isOrientationPreserving, const int face, const int vert)
    {
        MikktCustomData* customData = static_cast<MikktCustomData*>(context->m_pUserData);
        const AZ::u32 vertexIndex = customData->m_blendShapeData->GetFaceVertexIndex(face, vert);
        const float flipSign = isOrientationPreserving ? 1.0f : -1.0f;
        const AZ::Vector4 tangentVec(tangent[0]*magS, tangent[1]*magS, tangent[2]*magS, flipSign);
        const AZ::Vector3 bitangentVec(bitangent[0]*magT, bitangent[1]*magT, bitangent[2]*magT);

        // Set the tangent and bitangent back to the blend shape
        AZStd::vector<AZ::Vector4>& tangents = customData->m_blendShapeData->GetTangents();
        AZStd::vector<AZ::Vector3>& bitangents = customData->m_blendShapeData->GetBitangents();
        tangents[vertexIndex] = tangentVec;
        bitangents[vertexIndex] = bitangentVec;
    }

    void SetTSpaceBasic(const SMikkTSpaceContext* context, const float tangent[], const float signValue, const int face, const int vert)
    {
        MikktCustomData* customData = static_cast<MikktCustomData*>(context->m_pUserData);
        const AZ::u32 vertexIndex = customData->m_blendShapeData->GetFaceVertexIndex(face, vert);
        AZ::Vector3 tangentVec3(tangent[0], tangent[1], tangent[2]);
        tangentVec3.NormalizeSafe();
        AZ::Vector3 normal = customData->m_blendShapeData->GetNormal(vertexIndex);
        normal.NormalizeSafe();
        const AZ::Vector3 bitangent = normal.Cross(tangentVec3) * signValue;

        // Set the tangent and bitangent back to the blend shape
        AZStd::vector<AZ::Vector4>& tangents = customData->m_blendShapeData->GetTangents();
        AZStd::vector<AZ::Vector3>& bitangents = customData->m_blendShapeData->GetBitangents();
        tangents[vertexIndex] = AZ::Vector4(tangentVec3.GetX(), tangentVec3.GetY(), tangentVec3.GetZ(), signValue);
        bitangents[vertexIndex] = bitangent;
    }

    bool GenerateTangents(AZ::SceneData::GraphData::BlendShapeData* blendShapeData,
        size_t uvSetIndex,
        AZ::SceneAPI::DataTypes::MikkTSpaceMethod tSpaceMethod)
    {
        // Create tangent and bitangent data sets and relate them to the given UV set.
        const AZStd::vector<AZ::Vector2>& uvSet = blendShapeData->GetUVs(static_cast<AZ::u8>(uvSetIndex));
        if (uvSet.empty())
        {
            AZ_Error(AZ::SceneAPI::Utilities::ErrorWindow, false,
                "Cannot find UV data (set index=%d) to generate tangents and bitangents from in MikkT generator.\n",
                uvSetIndex);
            return false;
        }

        // Pre-allocate the tangent and bitangent data.
        AZStd::vector<AZ::Vector4>& tangents = blendShapeData->GetTangents();
        AZStd::vector<AZ::Vector3>& bitangents = blendShapeData->GetBitangents();
        tangents.resize(blendShapeData->GetVertexCount());
        bitangents.resize(blendShapeData->GetVertexCount());

        //----------------------------------

        // Provide the MikkT interface.
        SMikkTSpaceInterface mikkInterface;
        mikkInterface.m_getNumFaces         = GetNumFaces;
        mikkInterface.m_getNormal           = GetNormal;
        mikkInterface.m_getPosition         = GetPosition;
        mikkInterface.m_getTexCoord         = GetTexCoord;
        mikkInterface.m_getNumVerticesOfFace= GetNumVerticesOfFace;

        switch (tSpaceMethod)
        {
            case AZ::SceneAPI::DataTypes::MikkTSpaceMethod::TSpaceBasic:
            {
                mikkInterface.m_setTSpace = nullptr;
                mikkInterface.m_setTSpaceBasic = SetTSpaceBasic;
                break;
            }
            default:
            {
                mikkInterface.m_setTSpace = SetTSpace;
                mikkInterface.m_setTSpaceBasic = nullptr;
                break;
            }
        }

        // Set the MikkT custom data.
        MikktCustomData customData;
        customData.m_blendShapeData = blendShapeData;
        customData.m_uvSetIndex = uvSetIndex;

        // Generate the tangents.
        SMikkTSpaceContext mikkContext;
        mikkContext.m_pInterface    = &mikkInterface;
        mikkContext.m_pUserData     = &customData;
        if (genTangSpaceDefault(&mikkContext) == 0)
        {
            AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Failed to generate tangents and bitangents using MikkT for blend shape, because MikkT reported failure!\n");
            return false;
        }

        return true;
    }
} // namespace AZ::TangentGeneration::MikkT
