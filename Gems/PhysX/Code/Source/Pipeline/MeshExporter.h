/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Physics/Material/PhysicsMaterialSlots.h>

#include <SceneAPI/SceneCore/Components/ExportingComponent.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Events
        {
            class ExportEventContext;
        }

        namespace Containers
        {
            class Scene;
            class SceneGraph;
        }

        namespace DataTypes
        {
            class IMeshData;
        }
    }
}

namespace PhysX
{
    namespace Pipeline
    {
        class MeshGroup;

        class MeshExporter
            : public AZ::SceneAPI::SceneCore::ExportingComponent
        {
        public:
            AZ_COMPONENT(MeshExporter, "{4EA8B035-064D-456F-A9BA-0CDA40E9B84C}", AZ::SceneAPI::SceneCore::ExportingComponent);

            MeshExporter();
            ~MeshExporter() override = default;

            static void Reflect(AZ::ReflectContext* context);

            AZ::SceneAPI::Events::ProcessingResult ProcessContext(AZ::SceneAPI::Events::ExportEventContext& context) const;

        private:
            AZ::SceneAPI::Events::ProcessingResult ExportMeshObject(AZ::SceneAPI::Events::ExportEventContext& context, const AZStd::shared_ptr<const AZ::SceneAPI::DataTypes::IMeshData>& meshToExport, const AZStd::string& nodePath, const Pipeline::MeshGroup& pxMeshGroup) const;
        };

        namespace Utils
        {
            //! A struct to store the materials of the mesh nodes selected in a mesh group.
            struct AssetMaterialsData
            {
                //! Material names coming from the source scene file.
                AZStd::vector<AZStd::string> m_sourceSceneMaterialNames;

                //! Look-up table for sourceSceneMaterialNames.
                AZStd::unordered_map<AZStd::string, size_t> m_materialIndexByName;

                //! Map of mesh nodes to their list of material indices associated to each face.
                AZStd::unordered_map<AZStd::string, AZStd::vector<AZ::u16>> m_nodesToPerFaceMaterialIndices;
            };

            //! Returns the list of materials assigned to the triangles
            //! of the mesh nodes selected in a mesh group.
            AZStd::optional<AssetMaterialsData> GatherMaterialsFromMeshGroup(
                const MeshGroup& meshGroup,
                const AZ::SceneAPI::Containers::SceneGraph& sceneGraph);

            //! Function to update a list of physics material slots from a new list.
            //! All those new materials not found in the previous list will fallback to default physics material.
            void UpdateAssetPhysicsMaterials(
                const AZStd::vector<AZStd::string>& newMaterials,
                Physics::MaterialSlots& physicsMaterialSlots);
        } // namespace Utils
    } // namespace Pipeline
} // namespace PhysX
