// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include "${GemName}DataAssetSystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace ${GemName}
{
    AZ_COMPONENT_IMPL(${GemName}DataAssetSystemComponent, "${GemName}DataAssetSystemComponent", "{${Random_Uuid}}");

    void ${GemName}DataAssetSystemComponent::Activate()
    {
        // Register Generic Assets
        //auto* ${AssetName}Handler = aznew AzFramework::GenericAssetHandler<${AssetName}>("${AssetName}", "${AssetGroup}", "${FileExtension}");
        //${AssetName}Handler->Register();
        //m_assetHandlers.emplace_back(${AssetName}Handler);
    }

    void ${GemName}DataAssetSystemComponent::Deactivate()
    {
        //Unregister Data Assets
        for (auto& assetHandler : m_assetHandlers)
        {
            if(assetHandler)
            {
                if (AZ::Data::AssetManager::IsReady())
                {
                    AZ::Data::AssetManager::Instance().UnregisterHandler(assetHandler.get());
                }
            }
        }
        m_assetHandlers.clear();
    }

    void ${GemName}DataAssetSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        // Reflect the Assets Reflect Methods
        //${AssetName}::Reflect(context);
    }

    void ${GemName}DataAssetSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("${GemName}DataAssetSystemComponentService"));
    }

    void ${GemName}DataAssetSystemComponent::GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
    }

    void ${GemName}DataAssetSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void ${GemName}DataAssetSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }
} // namespace ${GemName}
