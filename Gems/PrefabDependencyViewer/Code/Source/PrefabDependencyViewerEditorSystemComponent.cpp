/* Copyright (c) Contributors to the Open 3D Engine Project
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <PrefabDependencyViewerEditorSystemComponent.h>
#include <PrefabDependencyViewerWidget.h>
#include <QMenu>
#include <LyViewPaneNames.h>
#include <PrefabDependencyTreeGenerator.h>

namespace PrefabDependencyViewer
{
    using Outcome = AZ::Outcome<PrefabDependencyTree, const char*>;

    InstanceEntityMapperInterface* PrefabDependencyViewerEditorSystemComponent::s_prefabEntityMapperInterface = nullptr;
    PrefabSystemComponentInterface* PrefabDependencyViewerEditorSystemComponent::s_prefabSystemComponentInterface = nullptr;

    const char* PrefabDependencyViewerEditorSystemComponent::s_prefabViewerTitle = "Prefab Dependencies Viewer";

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

    void PrefabDependencyViewerEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void PrefabDependencyViewerEditorSystemComponent::Activate()
    {
        s_prefabEntityMapperInterface = AZ::Interface<InstanceEntityMapperInterface>::Get();
        if (nullptr == s_prefabEntityMapperInterface)
        {
            AZ_Assert(false, "Prefab Dependency Viewer Gem - could not get PrefabEntityMapperInterface on PrefabIntegrationManager construction");
            return;
        }

        s_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
        if (nullptr == s_prefabSystemComponentInterface)
        {
            AZ_Assert(false, "Prefab Dependency Viewer Gem - could not get PrefabSystemComponentInterface on PrefabIntergrationManager construction.");
            return;
        }

        AzToolsFramework::EditorContextMenuBus::Handler::BusConnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        //AZ_Error("DependencyViewer", false, "The Prefab Dependency Viewer has booted up");
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

        // Prefab Dependency Viewer
        {
            if (selectedEntities.size() == 1)
            {
                AZ::EntityId selectedEntity = selectedEntities[0];
                InstanceOptionalReference optionalReference = s_prefabEntityMapperInterface->FindOwningInstance(selectedEntity);

                if (AZStd::nullopt == optionalReference || !optionalReference.has_value())
                {
                    AZ_Assert(false, "Prefab - Couldn't retrieve the owning Prefab Instance of the corresponding ContainerEntity");
                    return;
                }

                Instance& prefabInstance = optionalReference.value().get();
                const TemplateId& tid = prefabInstance.GetTemplateId();
                // PrefabDom& templateJSON = s_prefabSystemComponentInterface->FindTemplateDom(tid);

                QAction* dependencyViewerAction = menu->addAction(QObject::tr("View Dependencies"));
                QObject::connect(
                    dependencyViewerAction, &QAction::triggered, dependencyViewerAction,
                    [this, &tid]
                    {
                        ContextMenu_DisplayAssetDependencies(tid);
                    });
            }
        }
    }
    void PrefabDependencyViewerEditorSystemComponent::ContextMenu_DisplayAssetDependencies([[maybe_unused]]const TemplateId& tid)
    {
        /* AZ_TracePrintf("Prefab Dependency Viewer", "%s\n", typeid(this).name());
        AZ_TracePrintf("Prefab Dependency Viewer", "%s\n", typeid(prefabInstance).name());
        AZ_TracePrintf(
            "Prefab Dependency Viewer", "%s\n", s_prefabPublicInterface->GetOwningInstancePrefabPath(selectedEntity).c_str());
        AZ_TracePrintf("Prefab Dependency Viewer", "%s\n", prefabInstance.GetAbsoluteInstanceAliasPath().c_str());
        AZ_TracePrintf("Prefab Dependency Viewer", "%s\n", typeid(prefabInstance.GetTemplateId()).name());
        */
        AzToolsFramework::OpenViewPane(s_prefabViewerTitle);
        
        PrefabDependencyViewerInterface* window = AZ::Interface<PrefabDependencyViewerInterface>::Get();

        if (nullptr == window)
        {
            AZ_Assert(false, "Prefab - Can't get the pointer to the window.");
            return;
        }

        Outcome outcome = GenerateTreeAndSetRoot(tid);
        if (outcome.IsSuccess())
        {
            PrefabDependencyTree tree = outcome.GetValue();
            window->DisplayTree(tree); // prefabInstance);
        }
        
        /*
         AZStd::vector<AZ::Entity&> entities;
        prefabInstance.GetEntities(
            [&entities](AZStd::unique_ptr<AZ::Entity>& entity)
            {
                //AZ_TracePrintf("Prefab Dependency Viewer", "%s\n", typeid(entity).name());
                //AZ_TracePrintf("Prefab Dependency Viewer", "%s\n", typeid(entity.get()).name());
                // entities.push_back(*entity.get());
                return true;
            });

        AZ_TracePrintf("Prefab Dependency Viewer", "%d\n", entities.size());
        */
    }

    void PrefabDependencyViewerEditorSystemComponent::NotifyRegisterViews()
    {
        AzToolsFramework::RegisterViewPane<PrefabDependencyViewerWidget>(
            s_prefabViewerTitle, LyViewPane::CategoryTools, AzToolsFramework::ViewPaneOptions());
    }


} // namespace PrefabDependencyViewer
