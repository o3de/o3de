/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_set.h>

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>

#include <Components/ClothConfiguration.h>

namespace NvCloth
{
    class ClothComponentMesh;

    //! Class for in-editor Cloth Component.
    class EditorClothComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , public AZ::Render::MeshComponentNotificationBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(EditorClothComponent, "{2C99B4EF-8A5F-4585-89F9-86D50754DF7E}", AzToolsFramework::Components::EditorComponentBase);

        static void Reflect(AZ::ReflectContext* context);

        EditorClothComponent();
        ~EditorClothComponent();

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        const MeshNodeList& GetMeshNodeList() const;
        const AZStd::unordered_set<AZStd::string>& GetMeshNodesWithBackstopData() const;

        // EditorComponentBase overrides ...
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        // AZ::Component overrides ...
        void Activate() override;
        void Deactivate() override;

        // AZ::Render::MeshComponentNotificationBus::Handler overrides ...
        void OnModelReady(const AZ::Data::Asset<AZ::RPI::ModelAsset>& modelAsset, const AZ::Data::Instance<AZ::RPI::Model>& model) override;
        void OnModelPreDestroy() override;

    private:
        bool IsSimulatedInEditor() const;
        AZ::u32 OnSimulatedInEditorToggled();

        void OnConfigurationChanged();

        bool ContainsBackstopData(AssetHelper* assetHelper, const AZStd::string& meshNode) const;

        void UpdateConfigMeshNodeData();

        ClothConfiguration m_config;

        AZStd::unique_ptr<ClothComponentMesh> m_clothComponentMesh;

        // List of mesh nodes from the asset that contains cloth data.
        // This list is not serialized, it's compiled when the asset has been received via MeshComponentNotificationBus.
        MeshNodeList m_meshNodeList;

        AZStd::string m_lastKnownMeshNode;

        AZStd::unordered_set<AZStd::string> m_meshNodesWithBackstopData;

        bool m_simulateInEditor = false;
    };
} // namespace NvCloth
