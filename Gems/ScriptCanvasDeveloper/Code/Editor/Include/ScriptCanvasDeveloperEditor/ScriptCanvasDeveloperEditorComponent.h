/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <ScriptCanvas/Bus/ScriptCanvasBus.h>

class QMainWindow;

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
        void MainWindowCreationEvent(QMainWindow* mainWindow) override;
    };
}
