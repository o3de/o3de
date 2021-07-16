/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Components/ExportingComponent.h>
#include <SceneAPI/SceneCore/Components/RCExportingComponent.h>
#include <SceneAPI/SceneCore/Events/ProcessingResult.h>

namespace AZ
{
    namespace rapidxml
    {
        template<class Ch>
        class xml_node;
        template<class Ch>
        class xml_attribute;
        template<class Ch>
        class xml_document;
    }

    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IMaterialData;
            class ISceneNodeGroup;
            class IMeshAdvancedRule;
        }

        namespace Events
        {
            class PreExportEventContext;
        }

        namespace Export
        {
            //! Scene exporting component that exports materials to the cache if needed before any processing happens.
            class BaseMaterialExporterComponent
            {
            public:
                virtual ~BaseMaterialExporterComponent() = default;

            protected:
                //! Prepares for processing and exporting by looking at all the groups and generating materials for them in the temp dir
                //! if needed. If there's already a material in the source folder this step will be ignored.
                virtual Events::ProcessingResult ExportMaterialsToTempDir(Events::PreExportEventContext& context, bool registerProducts) const;
                // Gets the root path that all texture paths have to be relative to, which is usually the game projects root.
                virtual AZStd::string GetTextureRootPath() const;
            };

            class MaterialExporterComponent
                : public SceneCore::ExportingComponent
                , protected BaseMaterialExporterComponent
            {
            public:
                AZ_COMPONENT(MaterialExporterComponent, "{F49A1534-05D9-4153-A86E-BF329CAAB543}", SceneCore::ExportingComponent);

                MaterialExporterComponent();
                ~MaterialExporterComponent() override = default;

                Events::ProcessingResult ExportMaterials(Events::PreExportEventContext& context) const;

                static void Reflect(ReflectContext* context);
            };

            class RCMaterialExporterComponent
                : public SceneCore::RCExportingComponent
                , protected BaseMaterialExporterComponent
            {
            public:
                AZ_COMPONENT(RCMaterialExporterComponent, "{EB643AB1-E68E-4297-8334-BB458383A327}", SceneCore::RCExportingComponent);

                RCMaterialExporterComponent();
                ~RCMaterialExporterComponent() override = default;

                Events::ProcessingResult ExportMaterials(Events::PreExportEventContext& context) const;

                static void Reflect(ReflectContext* context);
            };

            class MtlMaterialExporter
            {
            public:
                enum class SaveMaterialResult
                {
                    Success,
                    Skipped,
                    Failure
                };

                virtual ~MtlMaterialExporter() = default;

                //! Save the material references in the given group to the material.
                SCENE_CORE_API SaveMaterialResult SaveMaterialGroup(const AZ::SceneAPI::DataTypes::ISceneNodeGroup& sceneNodeGroup, 
                    const SceneAPI::Containers::Scene& scene, const AZStd::string& textureRootPath);
                //! Add the material references in the given group to previously saved materials.
                SCENE_CORE_API SaveMaterialResult AppendMaterialGroup(const AZ::SceneAPI::DataTypes::ISceneNodeGroup& sceneNodeGroup, 
                    const SceneAPI::Containers::Scene& scene);
                //! Write a previously loaded/constructed material to disk.
                //! @param filePath An absolute path to the target file. Source control action should be done before calling this function.
                SCENE_CORE_API bool WriteToFile(const char* filePath, bool updateWithChanges);

            protected:
                struct MaterialInfo
                {
                    AZStd::shared_ptr<const AZ::SceneAPI::DataTypes::IMaterialData> m_materialData { nullptr };
                    bool m_usesVertexColoring { false };
                    int m_physicsMaterialFlags { 0 }; //Values of AZ::GFxFramework::EMaterialFlags
                    AZStd::string m_name;
                };

                struct MaterialGroup
                {
                    AZStd::vector<MaterialInfo> m_materials;
                    bool m_removeMaterials;
                    bool m_updateMaterials;
                };

                SCENE_CORE_API SaveMaterialResult BuildMaterialGroup(const AZ::SceneAPI::DataTypes::ISceneNodeGroup& sceneNodeGroup, const SceneAPI::Containers::Scene& scene);
                
                //! Writes the material group to disk.
                //! @param filePath The absolute path to the final destination.
                //! @param materialGroup The material group to be written to disk.
                //! @param updateWithChanges Whether or not to update the material file at filePath. If false, the file will be overwritten regardless of the settings in the material group.
                SCENE_CORE_API bool WriteMaterialFile(const char* filePath, MaterialGroup& materialGroup, bool updateWithChanges) const;

                SCENE_CORE_API bool UsesVertexColoring(const AZ::SceneAPI::DataTypes::ISceneNodeGroup& sceneNodeGroup, const SceneAPI::Containers::Scene& scene,
                    SceneAPI::Containers::SceneGraph::HierarchyStorageConstIterator materialNode) const;

                SCENE_CORE_API bool DoesMeshNodeHaveColorStreamChild(const SceneAPI::Containers::Scene& scene,
                    SceneAPI::Containers::SceneGraph::NodeIndex meshNode) const;

                static const AZStd::string m_extension;

                SceneAPI::Containers::SceneGraph::NodeIndex* m_root;
                AZStd::string m_textureRootPath;
                MaterialGroup m_materialGroup;

                bool m_physicalize;
            };
        } // namespace Export
    } // namespace SceneAPI
} // namespace AZ
