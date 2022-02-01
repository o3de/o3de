/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ComponentEntityEditorPlugin.h"

#include <LyViewPaneNames.h>

#include "UI/QComponentEntityEditorMainWindow.h"
#include "UI/QComponentEntityEditorOutlinerWindow.h"
#include "UI/QComponentLevelEntityEditorMainWindow.h"
#include "UI/ComponentPalette/ComponentPaletteSettings.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/vector.h>

#include <AzFramework/API/ApplicationAPI.h>

#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/Slice/SliceRelationshipWidget.hxx>

#include "SandboxIntegration.h"
#include "Objects/ComponentEntityObject.h"

namespace ComponentEntityEditorPluginInternal
{
    void RegisterSandboxObjects()
    {
        GetIEditor()->GetClassFactory()->RegisterClass(new CTemplateObjectClassDesc<CComponentEntityObject>("ComponentEntity", "", "", OBJTYPE_AZENTITY, 201, "*.entity"));

        AZ::SerializeContext* serializeContext = nullptr;
        EBUS_EVENT_RESULT(serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
        AZ_Assert(serializeContext, "Serialization context not available");

        if (serializeContext)
        {
            ComponentPaletteSettings::Reflect(serializeContext);
        }
    }

    void UnregisterSandboxObjects()
    {
        GetIEditor()->GetClassFactory()->UnregisterClass("ComponentEntity");
    }


    void CheckComponentDeclarations()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!serializeContext)
        {
            return;
        }

        // Catch the common mistake of reflecting a Component to SerializeContext
        // without declaring how it inherits from AZ::Component.

        // Collect violators so we can list them all in one message, rather than raising N popups.
        AZStd::vector<AZ::ComponentDescriptor*> componentsLackingBaseClass;

        AZ::EBusAggregateResults<AZ::ComponentDescriptor*> allComponentDescriptors;
        AZ::ComponentDescriptorBus::BroadcastResult(allComponentDescriptors, &AZ::ComponentDescriptorBus::Events::GetDescriptor);
        for (AZ::ComponentDescriptor* componentDescriptor : allComponentDescriptors.values)
        {
            const AZ::TypeId& componentTypeId = componentDescriptor->GetUuid();
            const AZ::TypeId& typeOfAZComponent = azrtti_typeid<AZ::Component>();

            if (serializeContext->FindClassData(componentTypeId))
            {
                if (!AZ::EntityUtils::CheckIfClassIsDeprecated(serializeContext, componentTypeId)
                    && !AZ::EntityUtils::CheckDeclaresSerializeBaseClass(serializeContext, typeOfAZComponent, componentTypeId))
                {
                    componentsLackingBaseClass.push_back(componentDescriptor);
                }
            }
        }

        AZStd::string message;

        for (AZ::ComponentDescriptor* componentDescriptor : componentsLackingBaseClass)
        {
            message.append(AZStd::string::format("- %s %s\n",
                componentDescriptor->GetName(),
                componentDescriptor->GetUuid().ToString<AZStd::string>().c_str()));
        }

        if (!message.empty())
        {
            message.insert(0, "Programmer error:\nClasses deriving from AZ::Component are not declaring their base class to SerializeContext.\n"
                              "This will cause unexpected behavior such as components shifting around, or duplicating themselves.\n"
                              "Affected components:\n");
            message.append("\nReflection code should look something like this:\n"
                "serializeContext->Class<MyComponent, AZ::Component, ... (other base classes, if any) ...>()"
                "\nMake sure the Reflect function is called for all base classes as well.");

            // this happens during startup, and its a programmer error - so during startup, make it an error, so it shows as a pretty noisy
            // popup box.  Its important that programmers fix this, before they submit their code, so that data corruption / data loss does 
            // not occur.
            AZ_Error("Serialize", false, message.c_str());
        }
    }
} // end namespace ComponentEntityEditorPluginInternal

ComponentEntityEditorPlugin::ComponentEntityEditorPlugin([[maybe_unused]] IEditor* editor)
    : m_registered(false)
{
    m_appListener = new SandboxIntegrationManager();
    m_appListener->Setup();

    using namespace AzToolsFramework;

    ViewPaneOptions inspectorOptions;
    inspectorOptions.canHaveMultipleInstances = true;
    inspectorOptions.preferedDockingArea = Qt::RightDockWidgetArea;
    RegisterViewPane<QComponentEntityEditorInspectorWindow>(
        LyViewPane::EntityInspector,
        LyViewPane::CategoryTools,
        inspectorOptions);

    ViewPaneOptions pinnedInspectorOptions;
    pinnedInspectorOptions.canHaveMultipleInstances = true;
    pinnedInspectorOptions.preferedDockingArea = Qt::NoDockWidgetArea;
    pinnedInspectorOptions.paneRect = QRect(50, 50, 400, 700);
    pinnedInspectorOptions.showInMenu = false;
    RegisterViewPane<QComponentEntityEditorInspectorWindow>(
        LyViewPane::EntityInspectorPinned,
        LyViewPane::CategoryTools,
        pinnedInspectorOptions);

    bool prefabSystemEnabled = false;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(prefabSystemEnabled, &AzFramework::ApplicationRequests::IsPrefabSystemEnabled);

    if (prefabSystemEnabled)
    {
        // Add the new Outliner to the Tools Menu

        ViewPaneOptions outlinerOptions;
        outlinerOptions.canHaveMultipleInstances = true;
        outlinerOptions.preferedDockingArea = Qt::LeftDockWidgetArea;

        RegisterViewPane<QEntityOutlinerWindow>(
            LyViewPane::EntityOutliner,
            LyViewPane::CategoryTools,
            outlinerOptions);
    }
    else
    {
        ViewPaneOptions levelInspectorOptions;
        levelInspectorOptions.canHaveMultipleInstances = false;
        levelInspectorOptions.preferedDockingArea = Qt::RightDockWidgetArea;
        levelInspectorOptions.paneRect = QRect(50, 50, 400, 700);
        RegisterViewPane<QComponentLevelEntityEditorInspectorWindow>(
            LyViewPane::LevelInspector, LyViewPane::CategoryTools, levelInspectorOptions);

        // Add the Legacy Outliner to the Tools Menu
        ViewPaneOptions outlinerOptions;
        outlinerOptions.canHaveMultipleInstances = true;
        outlinerOptions.preferedDockingArea = Qt::LeftDockWidgetArea;

        // this pane was originally introduced with this name, so layout settings are all saved with that name, despite the preview label being removed.
        outlinerOptions.saveKeyName = "Entity Outliner (PREVIEW)";

        RegisterViewPane<QComponentEntityEditorOutlinerWindow>(
            LyViewPane::EntityOutliner,
            LyViewPane::CategoryTools,
            outlinerOptions);

        AzToolsFramework::ViewPaneOptions options;
        options.preferedDockingArea = Qt::NoDockWidgetArea;
        RegisterViewPane<SliceRelationshipWidget>(LyViewPane::SliceRelationships, LyViewPane::CategoryTools, options);
    }

    ComponentEntityEditorPluginInternal::RegisterSandboxObjects();

    // Check for common mistakes in component declarations
    ComponentEntityEditorPluginInternal::CheckComponentDeclarations();

    m_registered = true;
}

void ComponentEntityEditorPlugin::Release()
{
    if (m_registered)
    {
        using namespace AzToolsFramework;

        UnregisterViewPane(LyViewPane::EntityInspector);
        UnregisterViewPane(LyViewPane::EntityOutliner);
        UnregisterViewPane(LyViewPane::EntityInspectorPinned);

        ComponentEntityEditorPluginInternal::UnregisterSandboxObjects();
    }

    m_appListener->Teardown();
    delete m_appListener;

    delete this;
}
