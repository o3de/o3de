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

#include <AzCore/Serialization/SerializeContext.h>
#include <${Name}EditorSystemComponent.h>

namespace ${SanitizedCppName}
{
    void ${SanitizedCppName}EditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<${SanitizedCppName}EditorSystemComponent, AZ::Component>()->Version(1);
        }
    }

    ${SanitizedCppName}EditorSystemComponent::${SanitizedCppName}EditorSystemComponent()
    {
        if (${SanitizedCppName}Interface::Get() == nullptr)
        {
            ${SanitizedCppName}Interface::Register(this);
        }
    }

    ${SanitizedCppName}EditorSystemComponent::~${SanitizedCppName}EditorSystemComponent()
    {
        if (${SanitizedCppName}Interface::Get() == this)
        {
            ${SanitizedCppName}Interface::Unregister(this);
        }
    }

    void ${SanitizedCppName}EditorSystemComponent::Activate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void ${SanitizedCppName}EditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
    }

} // namespace ${SanitizedCppName}
