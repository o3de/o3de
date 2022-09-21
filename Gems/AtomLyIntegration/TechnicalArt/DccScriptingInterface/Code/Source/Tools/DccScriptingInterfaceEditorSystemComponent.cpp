/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#include <AzCore/Serialization/SerializeContext.h>
#include "DccScriptingInterfaceEditorSystemComponent.h"

namespace DccScriptingInterface
{
    void DccScriptingInterfaceEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DccScriptingInterfaceEditorSystemComponent, AZ::Component>();
        }
    }

    DccScriptingInterfaceEditorSystemComponent::DccScriptingInterfaceEditorSystemComponent()
    {
        if (DccScriptingInterfaceInterface::Get() == nullptr)
        {
            DccScriptingInterfaceInterface::Register(this);
        }
    }

    DccScriptingInterfaceEditorSystemComponent::~DccScriptingInterfaceEditorSystemComponent()
    {
        if (DccScriptingInterfaceInterface::Get() == this)
        {
            DccScriptingInterfaceInterface::Unregister(this);
        }
    }

    void DccScriptingInterfaceEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("DccScriptingInterfaceEditorService"));
    }

    void DccScriptingInterfaceEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("DccScriptingInterfaceEditorService"));
    }

    void DccScriptingInterfaceEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void DccScriptingInterfaceEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void DccScriptingInterfaceEditorSystemComponent::Activate()
    {
        DccScriptingInterfaceRequestBus::Handler::BusConnect();
    }

    void DccScriptingInterfaceEditorSystemComponent::Deactivate()
    {
        DccScriptingInterfaceRequestBus::Handler::BusDisconnect();
    }

} // namespace DccScriptingInterface
