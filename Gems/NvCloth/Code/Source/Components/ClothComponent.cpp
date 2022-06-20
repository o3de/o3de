/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Console/Console.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>

#include <Components/ClothComponent.h>

namespace NvCloth
{
    void ClothComponent::Reflect(AZ::ReflectContext* context)
    {
        ClothConfiguration::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ClothComponent, AZ::Component>()
                ->Version(0)
                ->Field("ClothConfiguration", &ClothComponent::m_config)
                ;
        }
    }

    ClothComponent::ClothComponent(const ClothConfiguration& config)
        : m_config(config)
    {
    }
    
    void ClothComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ClothMeshService", 0x6ffcbca5));
    }
    
    void ClothComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("MeshService", 0x71d8a455));
        required.push_back(AZ_CRC("TransformService", 0x8ee22c50));
    }

    void ClothComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void ClothComponent::Activate()
    {
        // Cloth components do not run on dedicated servers.
        if (auto* console = AZ::Interface<AZ::IConsole>::Get())
        {
            bool isDedicated = false;
            if (const auto result = console->GetCvarValue("sv_isDedicated", isDedicated);
                result == AZ::GetValueResult::Success && isDedicated)
            {
                return;
            }
        }

        AZ::Render::MeshComponentNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void ClothComponent::Deactivate()
    {
        AZ::Render::MeshComponentNotificationBus::Handler::BusDisconnect();

        m_clothComponentMesh.reset();
    }

    void ClothComponent::OnModelReady(
        const AZ::Data::Asset<AZ::RPI::ModelAsset>& asset,
        [[maybe_unused]] const AZ::Data::Instance<AZ::RPI::Model>& model)
    {
        if (!asset.IsReady())
        {
            return;
        }

        m_clothComponentMesh = AZStd::make_unique<ClothComponentMesh>(GetEntityId(), m_config);
    }

    void ClothComponent::OnModelPreDestroy()
    {
        m_clothComponentMesh.reset();
    }
} // namespace NvCloth
