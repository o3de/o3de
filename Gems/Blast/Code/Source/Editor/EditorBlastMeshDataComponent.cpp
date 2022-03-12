/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <API/ToolsApplicationAPI.h>
#include <Atom/RPI.Public/Scene.h>
#include <AtomLyIntegration/CommonFeatures/Material/MaterialComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Editor/EditorBlastMeshDataComponent.h>

namespace Blast
{
    void EditorBlastMeshDataComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("BlastMeshDataService"));
    }

    void EditorBlastMeshDataComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
    }

    void EditorBlastMeshDataComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC("EditorVisibilityService", 0x90888caf));
    }

    void EditorBlastMeshDataComponent::GetIncompatibleServices(
        AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("BlastMeshDataService"));
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void EditorBlastMeshDataComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<EditorBlastMeshDataComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(5)
                ->Field("Show Mesh Assets", &EditorBlastMeshDataComponent::m_showMeshAssets)
                ->Field("Mesh Assets", &EditorBlastMeshDataComponent::m_meshAssets)
                ->Field("Blast Chunks", &EditorBlastMeshDataComponent::m_blastChunksAsset);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<EditorBlastMeshDataComponent>(
                      "Blast Family Mesh Data", "Used to keep track of mesh assets for a Blast family")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Destruction")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Box.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Box.png")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(
                        AZ::Edit::Attributes::HelpPageURL,
                        "https://o3de.org/docs/user-guide/components/reference/destruction/blast-family-mesh-data/")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::CheckBox, &EditorBlastMeshDataComponent::m_showMeshAssets,
                        "Show mesh assets", "Allows manual editing of mesh assets")
                    ->Attribute(
                        AZ::Edit::Attributes::ChangeNotify,
                        &EditorBlastMeshDataComponent::OnMeshAssetsVisibilityChanged)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorBlastMeshDataComponent::m_meshAssets, "Mesh assets",
                        "Mesh assets needed for each Blast chunk")
                    ->Attribute(
                        AZ::Edit::Attributes::Visibility, &EditorBlastMeshDataComponent::GetMeshAssetsVisibility)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorBlastMeshDataComponent::OnMeshAssetsChanged)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorBlastMeshDataComponent::m_blastChunksAsset, "Blast Chunks",
                        "Manifest override to fill out meshes and material")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorBlastMeshDataComponent::OnBlastChunksAssetChanged);
            }
        }
    }

    void EditorBlastMeshDataComponent::Activate()
    {
        AZ_PROFILE_FUNCTION(System);
        OnMeshAssetsChanged();

        m_meshFeatureProcessor =
            AZ::RPI::Scene::GetFeatureProcessorForEntity<AZ::Render::MeshFeatureProcessorInterface>(GetEntityId());
        RegisterModel();

        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        AZ::Render::MaterialComponentNotificationBus::Handler::BusConnect(GetEntityId());
        EditorComponentBase::Activate();
    }

    void EditorBlastMeshDataComponent::Deactivate()
    {
        AZ_PROFILE_FUNCTION(System);
        EditorComponentBase::Deactivate();
        AZ::Render::MaterialComponentNotificationBus::Handler::BusDisconnect(GetEntityId());
        AZ::TransformNotificationBus::Handler::BusDisconnect(GetEntityId());
        UnregisterModel();
    }

    void EditorBlastMeshDataComponent::OnBlastChunksAssetChanged()
    {
        if (!m_blastChunksAsset.GetId().IsValid())
        {
            return;
        }

        using namespace AZ::Data;
        const AssetId blastAssetId = m_blastChunksAsset.GetId();
        m_blastChunksAsset = AssetManager::Instance().GetAsset<BlastChunksAsset>(blastAssetId, AssetLoadBehavior::QueueLoad);
        m_blastChunksAsset.BlockUntilLoadComplete();

        if (!m_blastChunksAsset.Get() || m_blastChunksAsset.Get()->GetModelAssetIds().empty())
        {
            AZ_Warning("blast", false, "Blast Chunk Asset does not contain any models.")
            return;
        }

        // load up the new mesh list
        m_meshAssets.clear();
        for (const auto& meshId : m_blastChunksAsset.Get()->GetModelAssetIds())
        {
            auto meshAsset = AssetManager::Instance().GetAsset<AZ::RPI::ModelAsset>(meshId, AssetLoadBehavior::QueueLoad);
            if (meshAsset)
            {
                m_meshAssets.push_back(meshAsset);
            }
        }

        UnregisterModel();
        RegisterModel();

        using namespace AzToolsFramework;
        ToolsApplicationEvents::Bus::Broadcast(&ToolsApplicationEvents::InvalidatePropertyDisplay, Refresh_EntireTree);
    }

    void EditorBlastMeshDataComponent::OnMeshAssetsChanged()
    {
        for (auto& meshAsset : m_meshAssets)
        {
            meshAsset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::QueueLoad);
        }
        UnregisterModel();
        RegisterModel();
    }

    AZ::Crc32 EditorBlastMeshDataComponent::GetMeshAssetsVisibility() const
    {
        return m_showMeshAssets ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    void EditorBlastMeshDataComponent::OnMeshAssetsVisibilityChanged()
    {
        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree);
    }

    void EditorBlastMeshDataComponent::HandleModelChange(AZ::Data::Instance<AZ::RPI::Model> model)
    {
        if (model)
        {
            AZ::Render::MeshComponentNotificationBus::Event(
                GetEntityId(), &AZ::Render::MeshComponentNotificationBus::Events::OnModelReady, model->GetModelAsset(),
                model);
            AZ::Render::MaterialReceiverNotificationBus::Event(
                GetEntityId(), &AZ::Render::MaterialReceiverNotificationBus::Events::OnMaterialAssignmentsChanged);
        }
    }

    void EditorBlastMeshDataComponent::RegisterModel()
    {
        if (m_meshFeatureProcessor && !m_meshAssets.empty() && m_meshAssets[0].GetId().IsValid())
        {
            AZ::Render::MaterialAssignmentMap materials;
            AZ::Render::MaterialComponentRequestBus::EventResult(
                materials, GetEntityId(), &AZ::Render::MaterialComponentRequests::GetMaterialOverrides);

            m_meshFeatureProcessor->ReleaseMesh(m_meshHandle);
            m_meshHandle = m_meshFeatureProcessor->AcquireMesh(AZ::Render::MeshHandleDescriptor{ m_meshAssets[0] }, materials);
            m_meshFeatureProcessor->ConnectModelChangeEventHandler(m_meshHandle, m_changeEventHandler);

            HandleModelChange(m_meshFeatureProcessor->GetModel(m_meshHandle));

            AZ::Transform transform = AZ::Transform::Identity();
            AZ::TransformBus::EventResult(transform, GetEntityId(), &AZ::TransformInterface::GetWorldTM);
            m_meshFeatureProcessor->SetTransform(m_meshHandle, transform);
        }
    }

    void EditorBlastMeshDataComponent::UnregisterModel()
    {
        if (m_meshFeatureProcessor)
        {
            m_meshFeatureProcessor->ReleaseMesh(m_meshHandle);
        }
    }

    void EditorBlastMeshDataComponent::BuildGameEntity([[maybe_unused]] AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<BlastMeshDataComponent>(m_meshAssets);
    }

    const AZ::Data::Asset<BlastChunksAsset>& EditorBlastMeshDataComponent::GetBlastChunksAsset() const
    {
        return m_blastChunksAsset;
    }

    const AZStd::vector<AZ::Data::Asset<AZ::RPI::ModelAsset>>& EditorBlastMeshDataComponent::GetMeshAssets() const
    {
        return m_meshAssets;
    }

    void EditorBlastMeshDataComponent::OnMaterialsUpdated([
        [maybe_unused]] const AZ::Render::MaterialAssignmentMap& materials)
    {
        UnregisterModel();
        RegisterModel();
    }

    void EditorBlastMeshDataComponent::OnTransformChanged(
        [[maybe_unused]] const AZ::Transform& local, const AZ::Transform& world)
    {
        if (m_meshFeatureProcessor)
        {
            m_meshFeatureProcessor->SetTransform(m_meshHandle, world);
        }
    }
} // namespace Blast
