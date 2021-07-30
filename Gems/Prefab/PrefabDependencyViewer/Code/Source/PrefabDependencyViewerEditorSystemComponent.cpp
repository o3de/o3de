/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include <AzToolsFramework/API/ViewPaneOptions.h>

#include <LyViewPaneNames.h>

#include <MainWindow.h>

#include <PrefabDependencyTree.h>
#include <PrefabDependencyViewerEditorSystemComponent.h>

#include <QMenu>

namespace PrefabDependencyViewer
{
    using Outcome = AZ::Outcome<PrefabDependencyTree, const char*>;

    void PrefabDependencyViewerEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PrefabDependencyViewerEditorSystemComponent, AZ::Component>()
                ->Version(0);
        }
    }

    PrefabDependencyViewerEditorSystemComponent::PrefabDependencyViewerEditorSystemComponent() = default;

    void PrefabDependencyViewerEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PrefabDependencyViewerEditorService"));
    }

    void PrefabDependencyViewerEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PrefabDependencyViewerEditorService"));
    }

    void PrefabDependencyViewerEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void PrefabDependencyViewerEditorSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("PrefabSystem"));
        dependent.push_back(AZ_CRC_CE("EditorEntityContextService"));
    }

    void PrefabDependencyViewerEditorSystemComponent::Activate()
    {
        m_prefabEntityMapperInterface = AZ::Interface<InstanceEntityMapperInterface>::Get();
        if (nullptr == m_prefabEntityMapperInterface)
        {
            // Since the Viewer is listed as "Tools", it might be loaded into Tools that
            // are not in the Editor. => Shouldn't assert in light of that situation.
            AZ_Error("Prefab Dependency Viewer", false,
                "could not get InstanceEntityMapperInterface during it's EditorSystemComponent activation.")
            return;
        }

        m_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
        if (nullptr == m_prefabSystemComponentInterface)
        {
            AZ_Error("Prefab Dependency Viewer", false,
                "could not get PrefabSystemComponentInterface during it's EditorSystemComponent activation.");
            return;
        }

        m_prefabPublicInterface = AZ::Interface<PrefabPublicInterface>::Get();
        if (nullptr == m_prefabPublicInterface)
        {
            AZ_Error("Prefab Dependency Viewer", false,
                "could not get PrefabPublicInterface during it's EditorSystemComponent activation.");
            return;
        }

        AzToolsFramework::EditorContextMenuBus::Handler::BusConnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void PrefabDependencyViewerEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorContextMenuBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
    }
    
    int PrefabDependencyViewerEditorSystemComponent::GetMenuPosition() const
    {
        return aznumeric_cast<int>(AzToolsFramework::EditorContextMenuOrdering::BOTTOM);
    }

    AZStd::string PrefabDependencyViewerEditorSystemComponent::GetMenuIdentifier() const
    {
        return s_prefabViewerTitle;
    }

    void PrefabDependencyViewerEditorSystemComponent::PopulateEditorGlobalContextMenu(
        QMenu* menu, [[maybe_unused]] const AZ::Vector2& point, [[maybe_unused]] int flags)
    {
        AzToolsFramework::EntityIdList selectedEntities;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(
            selectedEntities, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);

        if (nullptr == m_prefabPublicInterface        ||
            nullptr == m_prefabEntityMapperInterface  ||
            nullptr == m_prefabSystemComponentInterface)
        {
            AZ_Error("Prefab Dependency Viewer", false, "one of the required interfaces is a nullptr.");
            return;
        }
        if (selectedEntities.size() == 1 &&
            m_prefabPublicInterface->IsInstanceContainerEntity(selectedEntities[0]))
        {
            AZ::EntityId selectedEntity = selectedEntities[0];
            InstanceOptionalReference optionalReference = m_prefabEntityMapperInterface->FindOwningInstance(selectedEntity);

            if (AZStd::nullopt == optionalReference || !optionalReference.has_value())
            {
                AZ_Error("Prefab Dependency Viewer", false,
                    "couldn't retrieve the owning Prefab Instance of the corresponding ContainerEntity");
                return;
            }

            Instance& prefabInstance = optionalReference.value().get();
            const TemplateId& tid = prefabInstance.GetTemplateId();

            QAction* dependencyViewerAction = menu->addAction(QObject::tr("View Dependencies"));
            QObject::connect(
                dependencyViewerAction, &QAction::triggered, dependencyViewerAction,
                [this, &tid]
                {
                    ContextMenu_DisplayAssetDependencies(tid);
                });
        }

    }

    void PrefabDependencyViewerEditorSystemComponent::ContextMenu_DisplayAssetDependencies(const TemplateId& tid)
    {
        AzToolsFramework::OpenViewPane(s_prefabViewerTitle);
        
        PrefabDependencyViewerInterface* window = AZ::Interface<PrefabDependencyViewerInterface>::Get();

        if (nullptr == window)
        {
            AZ_Error("Prefab Dependency Viewer", false, "Can't get the pointer to the window.");
            return;
        }

        Outcome outcome = GenerateTreeAndSetRoot(tid);
        if (outcome.IsSuccess())
        {
            PrefabDependencyTree tree = outcome.GetValue();
            window->DisplayTree(tree);
        }
        else
        {
            AZ_Error("Prefab Dependency Viewer", false, outcome.GetError());
        }
    }

    Outcome PrefabDependencyViewerEditorSystemComponent::GenerateTreeAndSetRoot(TemplateId tid)
    {
        return PrefabDependencyTree::GenerateTreeAndSetRoot(tid, m_prefabSystemComponentInterface);
    }

    void PrefabDependencyViewerEditorSystemComponent::NotifyRegisterViews()
    {
        AzToolsFramework::RegisterViewPane<PrefabDependencyViewerWidget>(
            s_prefabViewerTitle, LyViewPane::CategoryTools, AzToolsFramework::ViewPaneOptions());
    }
} // namespace PrefabDependencyViewer
