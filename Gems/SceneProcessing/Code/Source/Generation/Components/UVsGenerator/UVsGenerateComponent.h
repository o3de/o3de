/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/SceneCore/Components/GenerationComponent.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneData/Rules/UVsRule.h>
#include <AzCore/RTTI/RTTI.h>

namespace AZ::SceneAPI::DataTypes { class IMeshData; }
namespace AZ::SceneAPI::DataTypes { class IMeshVertexUVData; }
namespace AZ::SceneData::GraphData { class MeshVertexUVData;  }

namespace AZ::SceneGenerationComponents
{
    struct UVsGenerateContext
        : public AZ::SceneAPI::Events::ICallContext
    {
        AZ_RTTI(UVsGenerateContext, "{CC7301AB-A7EC-41FB-8BEE-DCC8C8C32BF4}", AZ::SceneAPI::Events::ICallContext)

        UVsGenerateContext(AZ::SceneAPI::Containers::Scene& scene)
            : m_scene(scene) {}
        UVsGenerateContext& operator=(const UVsGenerateContext& other) = delete;

        AZ::SceneAPI::Containers::Scene& GetScene() { return m_scene; }
        const AZ::SceneAPI::Containers::Scene& GetScene() const { return m_scene; }

    private:
        AZ::SceneAPI::Containers::Scene& m_scene;
    };

    //! Check whether UVs are to be generated, and if so, generate them.
    class UVsGenerateComponent
        : public AZ::SceneAPI::SceneCore::GenerationComponent
    {
    public:
        AZ_COMPONENT(UVsGenerateComponent, "{49121BDD-C7E5-4D39-89BC-28789C90057F}", AZ::SceneAPI::SceneCore::GenerationComponent);
        UVsGenerateComponent();

        static void Reflect(AZ::ReflectContext* context);

        // Invoked by the CallProcessorBinder flow.  This is essentially the entry point for this operation.
        AZ::SceneAPI::Events::ProcessingResult GenerateUVsData(UVsGenerateContext& context);
    private:
        bool GenerateUVsForMesh(
            AZ::SceneAPI::Containers::Scene& scene,
            const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex,
            AZ::SceneAPI::DataTypes::IMeshData* meshData,
            const AZ::SceneAPI::DataTypes::UVsGenerationMethod generationMethod,
            const bool replaceExisting);

        //! How many UV Sets already exist on the mesh?
        size_t CalcUvSetCount(AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex) const;

        //! find the Nth UV Set on the mesh and return it.
        AZ::SceneAPI::DataTypes::IMeshVertexUVData* FindUvData(AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex, AZ::u64 uvSet) const;

        //! Return the UV Rule (the modifier on the mesh group) or nullptr if no such modifier is applied.
        const AZ::SceneAPI::SceneData::UVsRule* GetUVsRule(const AZ::SceneAPI::Containers::Scene& scene) const;

        //! Create a new UV set and hook it into the scene graph
        bool CreateUVsLayer(
            AZ::SceneAPI::Containers::SceneManifest& manifest,
            const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex,
            AZ::SceneAPI::Containers::SceneGraph& graph,
            SceneData::GraphData::MeshVertexUVData** outUVsData);
    };
} // namespace AZ::SceneGenerationComponents
