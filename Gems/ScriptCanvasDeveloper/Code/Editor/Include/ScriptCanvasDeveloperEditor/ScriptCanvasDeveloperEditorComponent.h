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

#include <AzCore/Component/Component.h>
#include <ScriptCanvas/Bus/ScriptCanvasBus.h>

namespace ScriptCanvasDeveloperEditor
{
    class SystemComponent
        : public AZ::Component
        , private ScriptCanvasEditor::UINotificationBus::Handler
    {
    public:
        AZ_COMPONENT(SystemComponent, "{5360A16B-1918-42B7-8769-D2EBD97CB1DB}");

        SystemComponent() = default;

        static void Reflect(AZ::ReflectContext* context);

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

        // AZ::Component 
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////

        // ScriptCanvasEditor::UINotificationBus
        void MainWindowCreationEvent(QWidget*) override;
    };
}
