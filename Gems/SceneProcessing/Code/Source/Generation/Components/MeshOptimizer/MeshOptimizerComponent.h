/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/tuple.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/Components/GenerationComponent.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Events/ProcessingResult.h>

namespace AZ { class ReflectContext; }
namespace AZ::SceneAPI::DataTypes { class IBlendShapeData; }
namespace AZ::SceneAPI::DataTypes { class IMeshData; }
namespace AZ::SceneAPI::DataTypes { class IMeshGroup; }
namespace AZ::SceneAPI::DataTypes { class IMeshVertexUVData; }
namespace AZ::SceneAPI::DataTypes { class IMeshVertexTangentData; }
namespace AZ::SceneAPI::DataTypes { class IMeshVertexBitangentData; }
namespace AZ::SceneAPI::DataTypes { class ISkinWeightData; }
namespace AZ::SceneAPI::DataTypes { class IMeshVertexColorData; }
namespace AZ::SceneAPI::Events { class GenerateSimplificationEventContext; }
namespace AZ::SceneData::GraphData { class BlendShapeData; }
namespace AZ::SceneData::GraphData { class MeshData; }
namespace AZ::SceneData::GraphData { class MeshVertexBitangentData; }
namespace AZ::SceneData::GraphData { class MeshVertexColorData; }
namespace AZ::SceneData::GraphData { class MeshVertexTangentData; }
namespace AZ::SceneData::GraphData { class MeshVertexUVData; }
namespace AZ::SceneAPI::Containers { class SceneGraph; }

namespace AZ::SceneGenerationComponents
{
    class MeshOptimizerComponent
        : public AZ::SceneAPI::SceneCore::GenerationComponent
    {
    public:
        AZ_COMPONENT(MeshOptimizerComponent, "{05791580-A464-436C-B3EA-36AD68A42BC8}", AZ::SceneAPI::SceneCore::GenerationComponent)

        MeshOptimizerComponent();

        static void Reflect(AZ::ReflectContext* context);

        AZ::SceneAPI::Events::ProcessingResult OptimizeMeshes(AZ::SceneAPI::Events::GenerateSimplificationEventContext& context) const;

        static bool HasAnyBlendShapeChild(const AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex);

    private:
        template<class MeshDataType>
        static AZStd::tuple<
            AZStd::unique_ptr<MeshDataType>,
            AZStd::vector<AZStd::unique_ptr<AZ::SceneData::GraphData::MeshVertexUVData>>,
            AZStd::vector<AZStd::unique_ptr<AZ::SceneData::GraphData::MeshVertexTangentData>>,
            AZStd::vector<AZStd::unique_ptr<AZ::SceneData::GraphData::MeshVertexBitangentData>>,
            AZStd::vector<AZStd::unique_ptr<AZ::SceneData::GraphData::MeshVertexColorData>>,
            AZStd::unique_ptr<AZ::SceneAPI::DataTypes::ISkinWeightData>
        > OptimizeMesh(
            const MeshDataType* meshData,
            const SceneAPI::DataTypes::IMeshData* baseMesh,
            const AZStd::vector<AZStd::reference_wrapper<const AZ::SceneAPI::DataTypes::IMeshVertexUVData>>& uvs,
            const AZStd::vector<AZStd::reference_wrapper<const AZ::SceneAPI::DataTypes::IMeshVertexTangentData>>& tangents,
            const AZStd::vector<AZStd::reference_wrapper<const AZ::SceneAPI::DataTypes::IMeshVertexBitangentData>>& bitangents,
            const AZStd::vector<AZStd::reference_wrapper<const AZ::SceneAPI::DataTypes::IMeshVertexColorData>>& vertexColors,
            const AZStd::vector<AZStd::reference_wrapper<const AZ::SceneAPI::DataTypes::ISkinWeightData>>& skinWeights,
            const AZ::SceneAPI::DataTypes::IMeshGroup& meshGroup,
            bool hasBlendShapes);

        static void AddFace(AZ::SceneData::GraphData::BlendShapeData* blendShape, unsigned int index1, unsigned int index2, unsigned int index3, unsigned int faceMaterialId);
        static void AddFace(AZ::SceneData::GraphData::MeshData* mesh, unsigned int index1, unsigned int index2, unsigned int index3, unsigned int faceMaterialId);
    };
} // namespace AZ::SceneGenerationComponents
