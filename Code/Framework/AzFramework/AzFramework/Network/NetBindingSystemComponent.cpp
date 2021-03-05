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

#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Network/NetBindingSystemComponent.h>

namespace AzFramework
{
    NetBindingSystemComponent::NetBindingSystemComponent()
    {
    }

    NetBindingSystemComponent::~NetBindingSystemComponent()
    {
    }

    void NetBindingSystemComponent::Activate()
    {
        NetBindingSystemImpl::Init();
    }

    void NetBindingSystemComponent::Deactivate()
    {
        NetBindingSystemImpl::Shutdown();
    }

    void NetBindingSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        NetBindingSystemImpl::Reflect(context);

        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<NetBindingSystemComponent, AZ::Component>()
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<NetBindingSystemComponent>(
                    "NetBinding System", "Performs network binding for game entities.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Engine")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ;
            }
        }
    }

    void NetBindingSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("NetBindingSystemService", 0xa0ad6656));
    }

    void NetBindingSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("NetBindingSystemService", 0xa0ad6656));
    }
} // namespace AzFramework
