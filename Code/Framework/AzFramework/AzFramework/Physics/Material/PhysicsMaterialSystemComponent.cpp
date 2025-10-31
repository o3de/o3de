/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <AzFramework/Asset/GenericAssetHandler.h>
#include <AzFramework/Physics/Material/PhysicsMaterialPropertyValue.h>
#include <AzFramework/Physics/Material/PhysicsMaterialAsset.h>
#include <AzFramework/Physics/Material/PhysicsMaterialSlots.h>
#include <AzFramework/Physics/Material/PhysicsMaterialSystemComponent.h>

namespace Physics
{
    void MaterialSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        MaterialPropertyValue::Reflect(context);
        MaterialAsset::Reflect(context);
        MaterialSlots::Reflect(context);

        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<Physics::MaterialSystemComponent, AZ::Component>()
                ->Version(1)
                ;
        }
    }

    void MaterialSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PhysicsMaterialService"));
    }

    void MaterialSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PhysicsMaterialService"));
    }

    void MaterialSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void MaterialSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("AssetDatabaseService"));
        dependent.push_back(AZ_CRC_CE("AssetCatalogService"));
    }

    void MaterialSystemComponent::Activate()
    {
        auto* materialAsset = aznew AzFramework::GenericAssetHandler<MaterialAsset>("Physics Material", MaterialAsset::AssetGroup, MaterialAsset::FileExtension);
        materialAsset->Register();
        m_assetHandlers.emplace_back(materialAsset);
    }

    void MaterialSystemComponent::Deactivate()
    {
        for (auto& assetHandler : m_assetHandlers)
        {
            if (auto materialAssetHandler = azrtti_cast<AzFramework::GenericAssetHandler<MaterialAsset>*>(assetHandler.get());
                materialAssetHandler != nullptr)
            {
                materialAssetHandler->Unregister();
            }
        }
        m_assetHandlers.clear();
    }
} // namespace Physics
