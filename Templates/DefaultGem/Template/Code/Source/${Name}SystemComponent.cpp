// {BEGIN_LICENSE}
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
// {END_LICENSE}

#include <${Name}SystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

namespace ${SanitizedCppName}
{
    void ${SanitizedCppName}SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<${SanitizedCppName}SystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<${SanitizedCppName}SystemComponent>("${SanitizedCppName}", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void ${SanitizedCppName}SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("${SanitizedCppName}Service"));
    }

    void ${SanitizedCppName}SystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("${SanitizedCppName}Service"));
    }

    void ${SanitizedCppName}SystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void ${SanitizedCppName}SystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    ${SanitizedCppName}SystemComponent::${SanitizedCppName}SystemComponent()
    {
        if (${SanitizedCppName}Interface::Get() == nullptr)
        {
            ${SanitizedCppName}Interface::Register(this);
        }
    }

    ${SanitizedCppName}SystemComponent::~${SanitizedCppName}SystemComponent()
    {
        if (${SanitizedCppName}Interface::Get() == this)
        {
            ${SanitizedCppName}Interface::Unregister(this);
        }
    }

    void ${SanitizedCppName}SystemComponent::Init()
    {
    }

    void ${SanitizedCppName}SystemComponent::Activate()
    {
        ${SanitizedCppName}RequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    void ${SanitizedCppName}SystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        ${SanitizedCppName}RequestBus::Handler::BusDisconnect();
    }

    void ${SanitizedCppName}SystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
    }

} // namespace ${SanitizedCppName}
