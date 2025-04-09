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
#include "UI/ComponentPalette/ComponentPaletteSettings.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/vector.h>

#include <AzFramework/API/ApplicationAPI.h>

#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include "SandboxIntegration.h"

namespace ComponentEntityEditorPluginInternal
{
    void RegisterSandboxObjects()
    {

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        AZ_Assert(serializeContext, "Serialization context not available");

        if (serializeContext)
        {
            ComponentPaletteSettings::Reflect(serializeContext);
        }
    }

    void UnregisterSandboxObjects()
    {
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

void ComponentPaletteSettings::Reflect(AZ::ReflectContext* context)
{
    AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
    if (serializeContext)
    {
        serializeContext->Class<ComponentPaletteSettings>()
            ->Version(1)
            ->Field("m_favorites", &ComponentPaletteSettings::m_favorites)
            ;
    }
}

ComponentEntityEditorPlugin::ComponentEntityEditorPlugin([[maybe_unused]] IEditor* editor)
    : m_registered(false)
{
    m_appListener = new SandboxIntegrationManager();
    m_appListener->Setup();

    using namespace AzToolsFramework;

    ViewPaneOptions inspectorOptions;
    inspectorOptions.canHaveMultipleInstances = true;
    inspectorOptions.preferedDockingArea = Qt::RightDockWidgetArea;
    // Override the default behavior for component mode enter/exit and imgui enter/exit
    // so that we don't automatically disable and enable the entire Entity Inspector. This will be handled separately per-component.
    inspectorOptions.isDisabledInComponentMode = false;
    inspectorOptions.isDisabledInImGuiMode = false;

    RegisterViewPane<QComponentEntityEditorInspectorWindow>(
        LyViewPane::Inspector,
        LyViewPane::CategoryTools,
        inspectorOptions);

    ViewPaneOptions pinnedInspectorOptions;
    pinnedInspectorOptions.canHaveMultipleInstances = true;
    pinnedInspectorOptions.preferedDockingArea = Qt::NoDockWidgetArea;
    pinnedInspectorOptions.paneRect = QRect(50, 50, 400, 700);
    pinnedInspectorOptions.showInMenu = false;
    // Override the default behavior for component mode enter/exit and imgui enter/exit
    // so that we don't automatically disable and enable the entire Pinned Entity Inspector. This will be handled separately per-component.
    pinnedInspectorOptions.isDisabledInComponentMode = false;
    pinnedInspectorOptions.isDisabledInImGuiMode = false;

    RegisterViewPane<QComponentEntityEditorInspectorWindow>(
        LyViewPane::EntityInspectorPinned,
        LyViewPane::CategoryTools,
        pinnedInspectorOptions);

    // Add the Outliner to the Tools Menu
    ViewPaneOptions outlinerOptions;
    outlinerOptions.canHaveMultipleInstances = true;
    outlinerOptions.preferedDockingArea = Qt::LeftDockWidgetArea;
    // Override the default behavior for component mode enter/exit and imgui enter/exit
    // so that we don't automatically disable and enable the Entity Outliner. This will be handled separately.
    outlinerOptions.isDisabledInComponentMode = false;
    outlinerOptions.isDisabledInImGuiMode = false;

    RegisterViewPane<QEntityOutlinerWindow>(
        LyViewPane::EntityOutliner,
        LyViewPane::CategoryTools,
        outlinerOptions);

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

        UnregisterViewPane(LyViewPane::Inspector);
        UnregisterViewPane(LyViewPane::EntityOutliner);
        UnregisterViewPane(LyViewPane::EntityInspectorPinned);

        ComponentEntityEditorPluginInternal::UnregisterSandboxObjects();
    }

    m_appListener->Teardown();
    delete m_appListener;

    delete this;
}
