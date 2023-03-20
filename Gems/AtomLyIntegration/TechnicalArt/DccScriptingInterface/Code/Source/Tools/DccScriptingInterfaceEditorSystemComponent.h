/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Component/Component.h>
#include <DccScriptingInterface/DccScriptingInterfaceBus.h>

namespace DccScriptingInterface
{
    /// System component for DccScriptingInterface editor
    class DccScriptingInterfaceEditorSystemComponent
        : public DccScriptingInterfaceRequestBus::Handler
        , public AZ::Component
    {
    public:
        AZ_COMPONENT(DccScriptingInterfaceEditorSystemComponent, "{2436FA2A-632D-4DD5-A5CB-1C692C8CB08B}");
        static void Reflect(AZ::ReflectContext* context);

        DccScriptingInterfaceEditorSystemComponent();
        ~DccScriptingInterfaceEditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate();
        void Deactivate();
    };
} // namespace DccScriptingInterface
