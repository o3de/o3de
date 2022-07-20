
/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#include <AzCore/Serialization/SerializeContext.h>
#include <MeshletsEditorSystemComponent.h>

namespace AZ
{
    namespace Meshlets
    {
        void MeshletsEditorSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<MeshletsEditorSystemComponent, MeshletsSystemComponent>()
                    ->Version(0);
            }
        }

        MeshletsEditorSystemComponent::MeshletsEditorSystemComponent() = default;

        MeshletsEditorSystemComponent::~MeshletsEditorSystemComponent() = default;

        void MeshletsEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            BaseSystemComponent::GetProvidedServices(provided);
            provided.push_back(AZ_CRC_CE("MeshletsEditorService"));
        }

        void MeshletsEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            BaseSystemComponent::GetIncompatibleServices(incompatible);
            incompatible.push_back(AZ_CRC_CE("MeshletsEditorService"));
        }

        void MeshletsEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            BaseSystemComponent::GetRequiredServices(required);
        }

        void MeshletsEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            BaseSystemComponent::GetDependentServices(dependent);
        }

        void MeshletsEditorSystemComponent::Activate()
        {
            MeshletsSystemComponent::Activate();
            AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        }

        void MeshletsEditorSystemComponent::Deactivate()
        {
            AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
            MeshletsSystemComponent::Deactivate();
        }

    } // namespace Meshlets
} // namespace AZ

