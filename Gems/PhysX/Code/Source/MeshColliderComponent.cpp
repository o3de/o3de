/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <PhysX_precompiled.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Physics/SystemBus.h>
#include <Source/MeshColliderComponent.h>
#include <Source/Utils.h>

namespace PhysX
{
    void MeshColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->ClassDeprecate("MeshColliderComponent", "{87A02711-8D7F-4966-87E1-77001EB6B29E}");
            serializeContext->Class<MeshColliderComponent, BaseColliderComponent>()
                ->Version(1)
            ;
        }
    }

    // AZ::Component
    void MeshColliderComponent::Activate()
    {
        bool validConfiguration = m_shapeConfigList.size() == 1;
        AZ_Error("PhysX", validConfiguration, "Expected exactly one collider/shape configuration for entity \"%s\".",
            GetEntity()->GetName().c_str());

        if (validConfiguration)
        {
            validConfiguration = m_shapeConfigList[0].second->GetShapeType() == Physics::ShapeType::PhysicsAsset;
            AZ_Error("PhysX", validConfiguration, "Expected shape configuration to be of type PhysicsAsset for entity \"%s\".",
                GetEntity()->GetName().c_str());
        }

        // only connect to buses if we have a valid configuration
        if (validConfiguration)
        {
            m_colliderConfiguration = m_shapeConfigList[0].first.get();
            m_shapeConfiguration = static_cast<Physics::PhysicsAssetShapeConfiguration*>(m_shapeConfigList[0].second.get());
            UpdateMeshAsset();
            MeshColliderComponentRequestsBus::Handler::BusConnect(GetEntityId());
            BaseColliderComponent::Activate();
        }
    }

    void MeshColliderComponent::Deactivate()
    {
        BaseColliderComponent::Deactivate();
        AZ::Data::AssetBus::MultiHandler::BusDisconnect();
        MeshColliderComponentRequestsBus::Handler::BusDisconnect();
        m_colliderConfiguration = nullptr;
        m_shapeConfiguration = nullptr;
    }

    // MeshColliderComponentRequestsBus
    AZ::Data::Asset<Pipeline::MeshAsset> MeshColliderComponent::GetMeshAsset() const
    {
        return AZ::Data::Asset<Pipeline::MeshAsset>(m_shapeConfiguration->m_asset.GetAs<Pipeline::MeshAsset>(),
            AZ::Data::AssetLoadBehavior::Default);
    }

    Physics::MaterialId MeshColliderComponent::GetMaterialId() const
    {
        return m_colliderConfiguration->m_materialSelection.GetMaterialId();
    }

    void MeshColliderComponent::SetMeshAsset(const AZ::Data::AssetId& id)
    {
        m_shapeConfiguration->m_asset.Create(id);
        UpdateMeshAsset();
    }

    void MeshColliderComponent::SetMaterialAsset(const AZ::Data::AssetId& id)
    {
        m_colliderConfiguration->m_materialSelection.SetMaterialLibrary(id);
    }

    void MeshColliderComponent::SetMaterialId(const Physics::MaterialId& id)
    {
        m_colliderConfiguration->m_materialSelection.SetMaterialId(id);
    }

    void MeshColliderComponent::UpdateMeshAsset()
    {
        if (m_shapeConfiguration->m_asset.GetId().IsValid())
        {
            AZ::Data::AssetBus::MultiHandler::BusConnect(m_shapeConfiguration->m_asset.GetId());
            m_shapeConfiguration->m_asset.QueueLoad();
        }
    }

    // AZ::Data::AssetBus::Handler
    void MeshColliderComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_shapeConfiguration->m_asset)
        {
            m_shapeConfiguration->m_asset = asset;

            Physics::SystemRequestBus::Broadcast(&Physics::SystemRequests::UpdateMaterialSelection, 
                *m_shapeConfiguration, *m_colliderConfiguration);
        }
    }

    void MeshColliderComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        if (asset == m_shapeConfiguration->m_asset)
        {
            m_shapeConfiguration->m_asset = asset;

            Physics::SystemRequestBus::Broadcast(&Physics::SystemRequests::UpdateMaterialSelection,
                *m_shapeConfiguration, *m_colliderConfiguration);
        }
    }

    // BaseColliderComponent
    void MeshColliderComponent::UpdateScaleForShapeConfigs()
    {
        if (m_shapeConfigList.size() != 1)
        {
            AZ_Error("PhysX Mesh Collider Component", false,
                "Expected exactly one collider/shape configuration for entity \"%s\".", GetEntity()->GetName().c_str());
            return;
        }

        m_shapeConfigList[0].second->m_scale = Utils::GetOverallScale(GetEntityId());
    }
} // namespace PhysX
