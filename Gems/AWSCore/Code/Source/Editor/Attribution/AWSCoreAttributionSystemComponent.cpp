/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Attribution/AWSCoreAttributionManager.h>
#include <Editor/Attribution/AWSCoreAttributionSystemComponent.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistry.h>

namespace AWSCore
{

    AWSAttributionSystemComponent::AWSAttributionSystemComponent()
        : m_manager(AZStd::make_unique<AWSAttributionManager>())
    {
    }

    void AWSAttributionSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AWSAttributionSystemComponent, AZ::Component>()->Version(0);
            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AWSAttributionSystemComponent>("AWSCoreAttributions", "Generates operation metrics for AWSCore gem")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "");
            }
        }
    }

    void AWSAttributionSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AWSCoreAttributionService"));
    }

    void AWSAttributionSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AWSCoreAttributionService"));
    }

    void AWSAttributionSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("AWSCoreService"));
    }

    void AWSAttributionSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void AWSAttributionSystemComponent::Init()
    {
        // load config if required - ie check if attributions should be generated and pass to manager
        m_manager->Init();
    }

    void AWSAttributionSystemComponent::Activate()
    {
    }

    void AWSAttributionSystemComponent::Deactivate()
    {
        m_manager.reset();
    }

} // namespace AWSCore

