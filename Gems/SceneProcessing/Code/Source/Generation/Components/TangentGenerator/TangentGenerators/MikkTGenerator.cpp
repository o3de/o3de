/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Generation/Components/TangentGenerator/TangentGenerators/MikkTGenerator.h>
#include <Generation/Components/TangentGenerator/TangentGenerateComponent.h>

#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexUVData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexTangentData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexBitangentData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexBitangentData.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexTangentData.h>
#include <SceneAPI/SceneData/Rules/TangentsRule.h>

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>

#include <mikkelsen/mikktspace.h>

namespace AZ::TangentGeneration::Mesh::MikkT
{
    // Returns the number of triangles in the mesh.
    int GetNumFaces(const SMikkTSpaceContext* context)
    {
        MikktCustomData* customData = static_cast<MikktCustomData*>(context->m_pUserData);
        return customData->m_meshData->GetFaceCount();
    }

    int GetNumVerticesOfFace(const SMikkTSpaceContext* context, const int face)
    {
        AZ_UNUSED(context);
        AZ_UNUSED(face);
        return 3;
    }

    void GetPosition(const SMikkTSpaceContext* context, float posOut[], const int face, const int vert)
    {
        MikktCustomData* customData = static_cast<MikktCustomData*>(context->m_pUserData);
        const AZ::u32 vertexIndex = customData->m_meshData->GetVertexIndex(face, vert);
        const AZ::Vector3& pos = customData->m_meshData->GetPosition(vertexIndex);
        posOut[0] = pos.GetX();
        posOut[1] = pos.GetY();
        posOut[2] = pos.GetZ();
    }

    void GetNormal(const SMikkTSpaceContext* context, float normOut[], const int face, const int vert)
    {
        MikktCustomData* customData = static_cast<MikktCustomData*>(context->m_pUserData);
        const AZ::u32 vertexIndex = customData->m_meshData->GetVertexIndex(face, vert);
        const AZ::Vector3 normal = customData->m_meshData->GetNormal(vertexIndex).GetNormalizedSafe();
        normOut[0] = normal.GetX();
        normOut[1] = normal.GetY();
        normOut[2] = normal.GetZ();
    }

    void GetTexCoord(const SMikkTSpaceContext* context, float texOut[], const int face, const int vert)
    {
        MikktCustomData* customData = static_cast<MikktCustomData*>(context->m_pUserData);
        const AZ::u32 vertexIndex = customData->m_meshData->GetVertexIndex(face, vert);
        const AZ::Vector2& uv = customData->m_uvData->GetUV(vertexIndex);
        texOut[0] = uv.GetX();
        texOut[1] = uv.GetY();
    }

    // This function is used to return the tangent and signValue to the application.
    // tangent is a unit length vector.
    // For normal maps it is sufficient to use the following simplified version of the bitangent which is generated at pixel/vertex level.
    // bitangent = signValue * cross(vN, tangent);
    // Note that the results are returned unindexed. It is possible to generate a new index list
    void SetTSpaceBasic(const SMikkTSpaceContext* context, const float tangent[], const float signValue, const int face, const int vert)
    {
        MikktCustomData* customData = static_cast<MikktCustomData*>(context->m_pUserData);
        const AZ::u32 vertexIndex = customData->m_meshData->GetVertexIndex(face, vert);
        AZ::Vector3 tangentVec3(tangent[0], tangent[1], tangent[2]);
        tangentVec3.NormalizeSafe();
        AZ::Vector3 normal = customData->m_meshData->GetNormal(vertexIndex);
        normal.NormalizeSafe();
        const AZ::Vector3 bitangent = normal.Cross(tangentVec3) * signValue;
        customData->m_tangentData->SetTangent(vertexIndex, AZ::Vector4(tangentVec3.GetX(), tangentVec3.GetY(), tangentVec3.GetZ(), signValue));
        customData->m_bitangentData->SetBitangent(vertexIndex, bitangent);
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
        const AZ::u32 vertexIndex = customData->m_meshData->GetVertexIndex(face, vert);
        const float flipSign = isOrientationPreserving ? 1.0f : -1.0f;
        const AZ::Vector4 tangentVec(tangent[0]*magS, tangent[1]*magS, tangent[2]*magS, flipSign);
        const AZ::Vector3 bitangentVec(bitangent[0]*magT, bitangent[1]*magT, bitangent[2]*magT);
        customData->m_tangentData->SetTangent(vertexIndex, tangentVec);
        customData->m_bitangentData->SetBitangent(vertexIndex, bitangentVec);
    }

    bool GenerateTangents(const AZ::SceneAPI::DataTypes::IMeshData* meshData,
        const AZ::SceneAPI::DataTypes::IMeshVertexUVData* uvData,
        AZ::SceneAPI::DataTypes::IMeshVertexTangentData* outTangentData,
        AZ::SceneAPI::DataTypes::IMeshVertexBitangentData* outBitangentData,
        AZ::SceneAPI::DataTypes::MikkTSpaceMethod tSpaceMethod)
    {
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
        customData.m_meshData       = meshData;
        customData.m_uvData         = uvData;
        customData.m_tangentData    = outTangentData;
        customData.m_bitangentData  = outBitangentData;

        // Generate the tangents.
        SMikkTSpaceContext mikkContext;
        mikkContext.m_pInterface    = &mikkInterface;
        mikkContext.m_pUserData     = &customData;
        if (genTangSpaceDefault(&mikkContext) == 0)
        {
            AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Failed to generate tangents and bitangents using MikkT.\n");
            return false;
        }

        return true;
    }
} // namespace AZ::TangentGeneration::MikkT
