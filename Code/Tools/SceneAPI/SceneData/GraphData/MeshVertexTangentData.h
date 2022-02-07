/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector4.h>
#include <AzCore/std/containers/vector.h>

#include <SceneAPI/SceneData/SceneDataConfiguration.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexTangentData.h>

namespace AZ::SceneData::GraphData
{
    class SCENE_DATA_CLASS MeshVertexTangentData
        : public AZ::SceneAPI::DataTypes::IMeshVertexTangentData
    {
    public:
        AZ_RTTI(MeshVertexTangentData, "{C16F0F38-8F8F-45A2-A33B-F2758922A7C4}", AZ::SceneAPI::DataTypes::IMeshVertexTangentData);

        static void Reflect(ReflectContext* context);

        SCENE_DATA_API ~MeshVertexTangentData() override = default;

        SCENE_DATA_API void CloneAttributesFrom(const IGraphObject* sourceObject) override;

        SCENE_DATA_API size_t GetCount() const override;
        SCENE_DATA_API const AZ::Vector4& GetTangent(size_t index) const override;
        SCENE_DATA_API void SetTangent(size_t vertexIndex, const AZ::Vector4& tangent) override;

        SCENE_DATA_API void SetTangentSetIndex(size_t setIndex) override;
        SCENE_DATA_API size_t GetTangentSetIndex() const override;

        SCENE_DATA_API AZ::SceneAPI::DataTypes::TangentGenerationMethod GetGenerationMethod() const override;
        SCENE_DATA_API void SetGenerationMethod(AZ::SceneAPI::DataTypes::TangentGenerationMethod method) override;

        SCENE_DATA_API void Resize(size_t numVerts);
        SCENE_DATA_API void ReserveContainerSpace(size_t numVerts);
        SCENE_DATA_API void AppendTangent(const AZ::Vector4& tangent);

        SCENE_DATA_API void GetDebugOutput(AZ::SceneAPI::Utilities::DebugOutput& output) const override;

    protected:
        AZStd::vector<AZ::Vector4> m_tangents;
        AZ::SceneAPI::DataTypes::TangentGenerationMethod m_generationMethod = AZ::SceneAPI::DataTypes::TangentGenerationMethod::FromSourceScene;
        size_t m_setIndex = 0;
    };
} // AZ::SceneData::GraphData
