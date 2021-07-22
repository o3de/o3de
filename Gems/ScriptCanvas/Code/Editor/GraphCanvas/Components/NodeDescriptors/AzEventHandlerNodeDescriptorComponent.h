/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Editor/GraphCanvas/Components/NodeDescriptors/NodeDescriptorComponent.h>

namespace ScriptCanvasEditor
{
    class AzEventHandlerNodeDescriptorComponent
        : public NodeDescriptorComponent
    {
    public:
        AZ_COMPONENT(AzEventHandlerNodeDescriptorComponent, "{BFD7B4D9-961F-4EEA-AFAC-09F10A419215}", NodeDescriptorComponent);
        static void Reflect(AZ::ReflectContext* reflectContext);

        AzEventHandlerNodeDescriptorComponent();
        AzEventHandlerNodeDescriptorComponent(AZStd::string eventName);
        ~AzEventHandlerNodeDescriptorComponent() = default;

        const AZStd::string& GetEventName() const;

    protected:

    private:
        AZStd::string m_eventName;
    };
}
