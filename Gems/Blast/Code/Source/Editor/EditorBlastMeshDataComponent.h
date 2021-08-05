/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Asset/BlastChunksAsset.h>
#include <Atom/Feature/Mesh/MeshFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentBus.h>
#include <AzCore/Component/TransformBus.h>
#include <Components/BlastMeshDataComponent.h>
#include <ToolsComponents/EditorComponentBase.h>

namespace Blast
{
    //! This class is a used for setting and storing meshes and material for chunks of an entity with
    //! Blast Family component during Editor time. It renders mesh of a root chunk in the viewport.
    class EditorBlastMeshDataComponent
        : public AzToolsFramework::Components::EditorComponentBase
        , public AZ::TransformNotificationBus::Handler
        , public AZ::Render::MaterialComponentNotificationBus::Handler
    {
    public:
        AZ_EDITOR_COMPONENT(
            EditorBlastMeshDataComponent, "{2DA6B11A-5091-423A-AC1D-7F03C46DBF43}",
            AzToolsFramework::Components::EditorComponentBase);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        static void Reflect(AZ::ReflectContext* context);

        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;

        // EditorComponentBase
        void BuildGameEntity(AZ::Entity* gameEntity) override;

        const AZ::Data::Asset<BlastChunksAsset>& GetBlastChunksAsset() const;
        const AZStd::vector<AZ::Data::Asset<AZ::RPI::ModelAsset>>& GetMeshAssets() const;

        void OnMaterialsUpdated(const AZ::Render::MaterialAssignmentMap& materials) override;
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

    private:
        void OnBlastChunksAssetChanged();
        void OnMeshAssetsChanged();
        AZ::Crc32 GetMeshAssetsVisibility() const;
        void OnMeshAssetsVisibilityChanged();

        void HandleModelChange(AZ::Data::Instance<AZ::RPI::Model> model);
        void RegisterModel();
        void UnregisterModel();

        //////////////////////////////////////////////////////////////////////////
        // Reflected data
        bool m_showMeshAssets = false;
        AZ::Data::Asset<BlastChunksAsset> m_blastChunksAsset;
        AZStd::vector<AZ::Data::Asset<AZ::RPI::ModelAsset>> m_meshAssets;
        //////////////////////////////////////////////////////////////////////////

        AZ::Render::MeshFeatureProcessorInterface* m_meshFeatureProcessor = nullptr;
        AZ::Render::MeshFeatureProcessorInterface::MeshHandle m_meshHandle;
        AZ::Render::MeshFeatureProcessorInterface::ModelChangedEvent::Handler m_changeEventHandler
            {[&](AZ::Data::Instance<AZ::RPI::Model> model)
             {
                 HandleModelChange(model);
             }};
    };
} // namespace Blast
