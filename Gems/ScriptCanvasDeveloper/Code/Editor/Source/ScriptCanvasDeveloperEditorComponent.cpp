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
#include "precompiled.h"

#include <AzToolsFramework/UI/PropertyEditor/GenericComboBoxCtrl.h>

#include <ScriptCanvas/Libraries/Libraries.h>
#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvasDeveloperEditor/ScriptCanvasDeveloperEditorComponent.h>
#include <ScriptCanvasDeveloperEditor/NodeListDumpAction.h>
#include <ScriptCanvasDeveloperEditor/TSGenerateAction.h>
#include <ScriptCanvasDeveloperEditor/AutomationActions/DynamicSlotFullCreation.h>
#include <ScriptCanvasDeveloperEditor/AutomationActions/NodePaletteFullCreation.h>
#include <ScriptCanvasDeveloperEditor/AutomationActions/VariableListFullCreation.h>
#include <ScriptCanvasDeveloperEditor/Developer.h>

namespace ScriptCanvasDeveloperEditor
{
    void SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        ScriptCanvasDeveloper::Libraries::Developer::Reflect(context);

        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void SystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("ScriptCanvasEditorService", 0x4fe2af98));
    }

    void SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ScriptCanvasDeveloperEditorService", 0x344d3e44));
    }

    void SystemComponent::Init()
    {
        AZ::EnvironmentVariable<ScriptCanvas::NodeRegistry> nodeRegistryVariable = AZ::Environment::FindVariable<ScriptCanvas::NodeRegistry>(ScriptCanvas::s_nodeRegistryName);
        if (nodeRegistryVariable)
        {
            ScriptCanvas::NodeRegistry& nodeRegistry = nodeRegistryVariable.Get();
            ScriptCanvasDeveloper::Libraries::Developer::InitNodeRegistry(nodeRegistry);
        }
    }

    void SystemComponent::Activate()
    {
        QWidget* mainWindow{};
        ScriptCanvasEditor::UIRequestBus::BroadcastResult(mainWindow, &ScriptCanvasEditor::UIRequests::GetMainWindow);
        if (mainWindow)
        {
            MainWindowCreationEvent(mainWindow);
        }
        ScriptCanvasEditor::UINotificationBus::Handler::BusConnect();

        AzToolsFramework::RegisterGenericComboBoxHandler<ScriptCanvas::Data::Type>();
    }

    void SystemComponent::Deactivate()
    {
        ScriptCanvasEditor::UINotificationBus::Handler::BusDisconnect();
    }

    void SystemComponent::MainWindowCreationEvent(QWidget* mainWindow)
    {
        NodeListDumpAction::CreateNodeListDumpAction(mainWindow);
        TSGenerateAction::SetupTSFileAction(mainWindow);
        DynamicSlotFullCreation::CreateDynamicSlotFullCreationAction(mainWindow);
        NodePaletteFullCreation::CreateNodePaletteFullCreationAction(mainWindow);
        VariablePaletteFullCreation::CreateVariablePaletteFullCreationAction(mainWindow);

    }
}
