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

#include <AzCore/Math/Vector4.h>
#include <AzCore/std/containers/vector.h>

#include <SceneAPI/SceneData/SceneDataConfiguration.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexTangentData.h>

namespace AZ
{
    namespace SceneData
    {
        namespace GraphData
        {

            class SCENE_DATA_CLASS MeshVertexTangentData
                : public AZ::SceneAPI::DataTypes::IMeshVertexTangentData
            {
            public:                
                AZ_RTTI(MeshVertexTangentData, "{C16F0F38-8F8F-45A2-A33B-F2758922A7C4}", AZ::SceneAPI::DataTypes::IMeshVertexTangentData);

                SCENE_DATA_API ~MeshVertexTangentData() override = default;

                SCENE_DATA_API size_t GetCount() const override;
                SCENE_DATA_API const AZ::Vector4& GetTangent(size_t index) const override;
                SCENE_DATA_API void SetTangent(size_t vertexIndex, const AZ::Vector4& tangent) override;

                SCENE_DATA_API void SetTangentSetIndex(size_t setIndex) override;
                SCENE_DATA_API size_t GetTangentSetIndex() const override;

                SCENE_DATA_API AZ::SceneAPI::DataTypes::TangentSpace GetTangentSpace() const override;
                SCENE_DATA_API void SetTangentSpace(AZ::SceneAPI::DataTypes::TangentSpace space) override;

                SCENE_DATA_API void Resize(size_t numVerts);
                SCENE_DATA_API void ReserveContainerSpace(size_t numVerts);
                SCENE_DATA_API void AppendTangent(const AZ::Vector4& tangent);

                SCENE_DATA_API void GetDebugOutput(AZ::SceneAPI::Utilities::DebugOutput& output) const override;
            protected:
                AZStd::vector<AZ::Vector4>              m_tangents;
                AZ::SceneAPI::DataTypes::TangentSpace   m_tangentSpace = AZ::SceneAPI::DataTypes::TangentSpace::FromFbx;
                size_t                                  m_setIndex = 0;
            };

        } // GraphData
    } // SceneData
} // AZ
