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

#include <ISystem.h>

#include <AzCore/Serialization/SerializeContext.h>

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

    void ClothComponent::Activate()
    {
        // Cloth components do not run on dedicated servers.
        AZ_Assert(gEnv, "Environment not ready");
        if (gEnv->IsDedicated())
        {
            return;
        }

        LmbrCentral::MeshComponentNotificationBus::Handler::BusConnect(GetEntityId());
    }

    void ClothComponent::Deactivate()
    {
        LmbrCentral::MeshComponentNotificationBus::Handler::BusDisconnect();

        m_clothComponentMesh.reset();
    }

    void ClothComponent::OnMeshCreated(const AZ::Data::Asset<AZ::Data::AssetData>& asset)
    {
        if (!asset.IsReady())
        {
            return;
        }

        m_clothComponentMesh = AZStd::make_unique<ClothComponentMesh>(GetEntityId(), m_config);
    }

    void ClothComponent::OnMeshDestroyed()
    {
        m_clothComponentMesh.reset();
    }
} // namespace NvCloth
