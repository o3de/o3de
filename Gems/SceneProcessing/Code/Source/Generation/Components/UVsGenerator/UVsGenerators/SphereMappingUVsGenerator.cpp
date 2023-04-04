/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Generation/Components/UVsGenerator/UVsGenerators/SphereMappingUVsGenerator.h>

#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexUVData.h>
#include <SceneAPI/SceneData/Rules/UVsRule.h>

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Aabb.h>

namespace AZ::UVsGeneration::Mesh::SphericalMapping
{
    bool GenerateUVsSphericalMapping(const AZ::SceneAPI::DataTypes::IMeshData* meshData, AZ::SceneData::GraphData::MeshVertexUVData* uvData)
    {
        uvData->Clear();
        unsigned int vertexCount = meshData->GetVertexCount();
        if (vertexCount == 0)
        {
            AZ_Trace(AZ::SceneAPI::Utilities::LogWindow, "Mesh has 0 vertex count, skipping UV generation.")
            return true;
        }

        uvData->ReserveContainerSpace(meshData->GetVertexCount());

        // calculate mesh center by bounding box center.
        AZ::Aabb meshAabb = AZ::Aabb::CreateNull();
        for (unsigned int vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
        {
            meshAabb.AddPoint(meshData->GetPosition(vertexIndex));
        }

        AZ::Vector3 centerPoint = meshAabb.GetCenter();
        for (unsigned int vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
        {
            // project to sphere.
            AZ::Vector3 projection = meshData->GetPosition(vertexIndex) - centerPoint;
            projection.Normalize();
            AZ::Vector2 uvCoords(asinf(projection.GetX()) / AZ::Constants::Pi + 0.5f,
                                 asinf(projection.GetY()) / AZ::Constants::Pi + 0.5f);
            uvData->AppendUV(uvCoords);
        }

        return true;
    }
} 
