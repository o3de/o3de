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
