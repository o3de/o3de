/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QMainWindow>
#include <QMenuBar>

#include <AzToolsFramework/UI/PropertyEditor/GenericComboBoxCtrl.h>

#include <ScriptCanvas/Libraries/Libraries.h>
#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvasDeveloperEditor/ScriptCanvasDeveloperEditorComponent.h>
#include <ScriptCanvasDeveloperEditor/NodeListDumpAction.h>
#include <ScriptCanvasDeveloperEditor/TSGenerateAction.h>
#include <ScriptCanvasDeveloperEditor/AutomationActions/DynamicSlotFullCreation.h>
#include <ScriptCanvasDeveloperEditor/AutomationActions/FullyConnectedNodePaletteCreation.h>
#include <ScriptCanvasDeveloperEditor/AutomationActions/NodePaletteFullCreation.h>
#include <ScriptCanvasDeveloperEditor/AutomationActions/VariableListFullCreation.h>
#include <ScriptCanvasDeveloperEditor/Developer.h>

#include <EditorAutomationTestDialog.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>

namespace ScriptCanvasDeveloperEditor
{
    ////////////////////
    // SystemComponent
    ////////////////////
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
        QMainWindow* mainWindow = nullptr;
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

    void SystemComponent::MainWindowCreationEvent(QMainWindow* mainWindow)
    {
        QMenuBar* menuBar = mainWindow->menuBar();

        QMenu* developerMenu = menuBar->addMenu("Developer");

        VariablePaletteFullCreation::CreateVariablePaletteFullCreationAction(developerMenu);

        developerMenu->addSeparator();

        NodePaletteFullCreation::CreateNodePaletteFullCreationAction(developerMenu);
        DynamicSlotFullCreation::CreateDynamicSlotFullCreationAction(developerMenu);

        developerMenu->addSeparator();

        NodeListDumpAction::CreateNodeListDumpAction(developerMenu);
        TSGenerateAction::SetupTSFileAction(developerMenu);

        developerMenu->addSeparator();

        QAction* action = developerMenu->addAction("Open Menu Test");

        QObject::connect(action, &QAction::triggered, [mainWindow]()
        {
            ScriptCanvasDeveloper::EditorAutomationTestDialogRequests* requests = ScriptCanvasDeveloper::EditorAutomationTestDialogRequestBus::FindFirstHandler(ScriptCanvasEditor::AssetEditorId);

            if (requests)
            {
                requests->ShowTestDialog();
            }
            else
            {
                ScriptCanvasDeveloper::EditorAutomationTestDialog* testDialog = new ScriptCanvasDeveloper::EditorAutomationTestDialog(mainWindow);
                testDialog->ShowTestDialog();
            }
        });

    }
}
