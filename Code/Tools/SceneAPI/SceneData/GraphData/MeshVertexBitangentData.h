/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/vector.h>

#include <SceneAPI/SceneData/SceneDataConfiguration.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexBitangentData.h>


namespace AZ
{
    namespace SceneData
    {
        namespace GraphData
        {

            class SCENE_DATA_CLASS MeshVertexBitangentData
                : public AZ::SceneAPI::DataTypes::IMeshVertexBitangentData
            {
            public:                
                AZ_RTTI(MeshVertexBitangentData, "{F56FB088-4C92-4453-AFE9-4E820F03FA90}", AZ::SceneAPI::DataTypes::IMeshVertexBitangentData);

                static void Reflect(ReflectContext* context);

                SCENE_DATA_API ~MeshVertexBitangentData() override = default;

                SCENE_DATA_API void CloneAttributesFrom(const IGraphObject* sourceObject) override;

                SCENE_DATA_API size_t GetCount() const override;
                SCENE_DATA_API const AZ::Vector3& GetBitangent(size_t index) const override;
                SCENE_DATA_API void SetBitangent(size_t vertexIndex, const AZ::Vector3& bitangent) override;

                SCENE_DATA_API void SetBitangentSetIndex(size_t setIndex) override;
                SCENE_DATA_API size_t GetBitangentSetIndex() const override;

                SCENE_DATA_API void Resize(size_t numVerts);
                SCENE_DATA_API void ReserveContainerSpace(size_t numVerts);
                SCENE_DATA_API void AppendBitangent(const AZ::Vector3& bitangent);

                SCENE_DATA_API AZ::SceneAPI::DataTypes::TangentSpace GetTangentSpace() const override;
                SCENE_DATA_API void SetTangentSpace(AZ::SceneAPI::DataTypes::TangentSpace space) override;

                SCENE_DATA_API void GetDebugOutput(AZ::SceneAPI::Utilities::DebugOutput& output) const override;
            protected:
                AZStd::vector<AZ::Vector3>              m_bitangents;
                AZ::SceneAPI::DataTypes::TangentSpace   m_tangentSpace = AZ::SceneAPI::DataTypes::TangentSpace::FromSourceScene;
                size_t                                  m_setIndex = 0;
            };

        } // GraphData
    } // SceneData
} // AZ
