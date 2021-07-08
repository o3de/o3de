/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/SceneCore/Components/GenerationComponent.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <AzCore/RTTI/RTTI.h>

namespace AZ::SceneAPI::DataTypes { class IMeshData; }
namespace AZ::SceneAPI::DataTypes { class IMeshVertexUVData; }
namespace AZ::SceneAPI::DataTypes { class IMeshVertexTangentData; }
namespace AZ::SceneAPI::DataTypes { class IMeshVertexBitangentData; }
namespace AZ::SceneAPI::DataTypes { enum class TangentSpace; }
namespace AZ::SceneData::GraphData
{
    class BlendShapeData;
}

namespace AZ::SceneGenerationComponents
{
    struct TangentGenerateContext
        : public AZ::SceneAPI::Events::ICallContext
    {
        AZ_RTTI(TangentGenerateContext, "{E836F8F8-5A66-497C-89CC-2D37D741CCAA}", AZ::SceneAPI::Events::ICallContext)

        TangentGenerateContext(AZ::SceneAPI::Containers::Scene& scene)
            : m_scene(scene) {}
        TangentGenerateContext& operator=(const TangentGenerateContext& other) = delete;

        AZ::SceneAPI::Containers::Scene& GetScene() { return m_scene; }
        const AZ::SceneAPI::Containers::Scene& GetScene() const { return m_scene; }

    private:
        AZ::SceneAPI::Containers::Scene& m_scene;
    };


    class TangentGenerateComponent
        : public AZ::SceneAPI::SceneCore::GenerationComponent
    {
    public:
        AZ_COMPONENT(TangentGenerateComponent, "{57743E6F-8718-491C-8A82-24A6763904F5}", AZ::SceneAPI::SceneCore::GenerationComponent);

        TangentGenerateComponent();

        static void Reflect(AZ::ReflectContext* context);
        static bool CreateTangentBitangentLayers(AZ::SceneAPI::Containers::SceneManifest& manifest, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex, size_t numVerts, size_t uvSetIndex, AZ::SceneAPI::DataTypes::TangentSpace tangentSpace,
            const char* spaceName, AZ::SceneAPI::Containers::SceneGraph& graph, AZ::SceneAPI::DataTypes::IMeshVertexTangentData** outTangentData, AZ::SceneAPI::DataTypes::IMeshVertexBitangentData** outBitangentData);

        AZ::SceneAPI::Events::ProcessingResult GenerateTangentData(TangentGenerateContext& context);

    private:
        void FindBlendShapes(
            AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex,
            AZStd::vector<AZ::SceneData::GraphData::BlendShapeData*>& outBlendShapes) const;
        bool GenerateTangentsForMesh(AZ::SceneAPI::Containers::Scene& scene, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex, AZ::SceneAPI::DataTypes::IMeshData* meshData);
        void UpdateFbxTangentWValues(AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex, const AZ::SceneAPI::DataTypes::IMeshData* meshData);
        AZStd::vector<AZ::SceneAPI::DataTypes::TangentSpace> CollectRequiredTangentSpaces(const AZ::SceneAPI::Containers::Scene& scene) const;
    };
} // namespace AZ::SceneGenerationComponents
