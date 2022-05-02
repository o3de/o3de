/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/EditorBlastFamilyComponent.h>

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <Blast/BlastSystemBus.h>
#include <Components/BlastFamilyComponent.h>

namespace Blast
{
    void EditorBlastFamilyComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<EditorBlastFamilyComponent, EditorComponentBase>()
                ->Version(2)
                ->Field("BlastAsset", &EditorBlastFamilyComponent::m_blastAsset)
                ->Field("BlastMaterialAsset", &EditorBlastFamilyComponent::m_blastMaterialAsset)
                ->Field("BlastMaterial", &EditorBlastFamilyComponent::m_legacyBlastMaterialId)
                ->Field("PhysicsMaterial", &EditorBlastFamilyComponent::m_physicsMaterialId)
                ->Field("ActorConfiguration", &EditorBlastFamilyComponent::m_actorConfiguration);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<EditorBlastFamilyComponent>(
                      "Blast Family", "Used to add a Blast family for destruction that will spawn Blast actors")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Destruction")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Box.png")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Box.png")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorBlastFamilyComponent::m_blastAsset, "Blast asset",
                        "Assigned blast asset")
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorBlastFamilyComponent::m_blastMaterialAsset, "Blast Material",
                        "Assigned blast material asset")
                        ->Attribute(AZ::Edit::Attributes::DefaultAsset, &EditorBlastFamilyComponent::GetDefaultBlastAssetId)
                        ->Attribute(AZ_CRC_CE("EditButton"), "")
                        ->Attribute(AZ_CRC_CE("EditDescription"), "Open in Asset Editor")
                        ->Attribute(AZ_CRC_CE("DisableEditButtonWhenNoAssetSelected"), true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorBlastFamilyComponent::m_physicsMaterialId,
                        "Physics Material", "Assigned physics material from current physics material library")
                    ->ElementAttribute(
                        Physics::Attributes::MaterialLibraryAssetId,
                        &EditorBlastFamilyComponent::GetPhysicsMaterialLibraryAssetId)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorBlastFamilyComponent::m_actorConfiguration,
                        "Actor configuration", "Configurations for actors in this family");
            }
        }
    }

    void EditorBlastFamilyComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("BlastFamilyService"));
    }

    void EditorBlastFamilyComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void EditorBlastFamilyComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("EditorVisibilityService"));
    }

    void EditorBlastFamilyComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("BlastFamilyService"));
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void EditorBlastFamilyComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_blastAsset)
        {
            m_blastAsset = asset;
        }
        else if (asset == m_blastMaterialAsset)
        {
            m_blastMaterialAsset = asset;
        }
    }

    void EditorBlastFamilyComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        OnAssetReady(asset);
    }

    void EditorBlastFamilyComponent::Activate()
    {
        AZ_PROFILE_FUNCTION(System);

        if (m_blastAsset.GetId().IsValid())
        {
            AZ::Data::AssetBus::MultiHandler::BusConnect(m_blastAsset.GetId());
            m_blastAsset.QueueLoad();
        }

        if (m_blastMaterialAsset.GetId().IsValid())
        {
            AZ::Data::AssetBus::MultiHandler::BusConnect(m_blastMaterialAsset.GetId());
            m_blastMaterialAsset.QueueLoad();
        }
    }

    void EditorBlastFamilyComponent::Deactivate()
    {
        AZ_PROFILE_FUNCTION(System);

        AZ::Data::AssetBus::MultiHandler::BusDisconnect();
    }

    void EditorBlastFamilyComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<BlastFamilyComponent>(
            m_blastAsset, m_blastMaterialAsset, m_physicsMaterialId, m_actorConfiguration);
    }

    AZ::Data::AssetId EditorBlastFamilyComponent::GetPhysicsMaterialLibraryAssetId() const
    {
        return AZ::Interface<AzPhysics::SystemInterface>::Get()->GetConfiguration()->m_materialLibraryAsset.GetId();
    }

    AZ::Data::AssetId EditorBlastFamilyComponent::GetDefaultBlastAssetId() const
    {
        // Used for Edit Context.
        // When the blast material asset property doesn't have an asset assigned it
        // will show "(default)" to indicate that the default material will be used.
        return {};
    }
} // namespace Blast
