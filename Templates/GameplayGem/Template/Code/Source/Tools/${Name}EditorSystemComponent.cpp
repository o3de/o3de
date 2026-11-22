// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include <Tools/${SanitizedCppName}EditorSystemComponent.h>
#include <${Name}/${SanitizedCppName}TypeIds.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace ${SanitizedCppName}
{
    AZ_COMPONENT_IMPL(${SanitizedCppName}EditorSystemComponent, "${SanitizedCppName}EditorSystemComponent", ${SanitizedCppName}EditorSystemComponentTypeId, BaseClass);

    void ${SanitizedCppName}EditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<${SanitizedCppName}EditorSystemComponent, ${SanitizedCppName}SystemComponent>()
                ->Version(1)
                ;
        }
    }

    void ${SanitizedCppName}EditorSystemComponent::Activate()
    {
        BaseClass::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void ${SanitizedCppName}EditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        BaseClass::Deactivate();
    }
} // namespace ${SanitizedCppName}
