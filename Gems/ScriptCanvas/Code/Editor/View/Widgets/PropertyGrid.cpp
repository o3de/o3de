/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <precompiled.h>
#include "PropertyGrid.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QScrollArea>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Math/Color.h>

#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/EntityPropertyEditor.hxx>

#include <AzToolsFramework/UI/PropertyEditor/ComponentEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/ComponentEditorHeader.hxx>

#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/ToolsComponents/EditorInspectorComponentBus.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

#include <Editor/View/Widgets/PropertyGridContextMenu.h>
#include <ScriptCanvas/Bus/RequestBus.h>

#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <GraphCanvas/Components/GraphCanvasPropertyBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Editor/GraphCanvasProfiler.h>

#include <ScriptCanvas/Core/Endpoint.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>

#include <ScriptCanvas/Libraries/Core/EBusEventHandler.h>
#include <ScriptCanvas/Libraries/Core/Method.h>

#include <ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>

namespace
{
    using StringToInstanceMap = AZStd::unordered_map<AZStd::string, ScriptCanvasEditor::Widget::PropertyGrid::InstancesToDisplay>;

    AZStd::string GetTitle(const AZ::EntityId& entityId, AZ::Component* instance)
    {
        AZStd::string result;

        AZStd::string title;
        GraphCanvas::NodeTitleRequestBus::EventResult(title, entityId, &GraphCanvas::NodeTitleRequests::GetTitle);

        AZStd::string subtitle;
        GraphCanvas::NodeTitleRequestBus::EventResult(subtitle, entityId, &GraphCanvas::NodeTitleRequests::GetSubTitle);
        
        // NOT a variable.
        result = title;

        if (!subtitle.empty())
        {
            result += (result.empty() ? "" : " - " ) + subtitle;
        }

        if (result.empty())
        {
            result = AzToolsFramework::GetFriendlyComponentName(instance).c_str();
        }

        return result;
    }

    void AddInstancesToComponentEditor(
        AzToolsFramework::ComponentEditor* componentEditor,
        const AZStd::list<AZ::Component*>& instanceList,
        AZStd::unordered_map<AZ::TypeId, AZ::Component*>& firstOfTypeMap,
        AZStd::unordered_set<AZ::EntityId>& entitySet)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        for (auto& instance : instanceList)
        {
            GRAPH_CANVAS_DETAILED_PROFILE_SCOPE("AddInstanceToComponentEditor::InnerLoop");
            // non-first instances are aggregated under the first instance
            AZ::Component* aggregateInstance = nullptr;
            if (firstOfTypeMap.count(instance->RTTI_GetType()) > 0)
            {
                aggregateInstance = firstOfTypeMap[instance->RTTI_GetType()];
            }
            else
            {
                firstOfTypeMap[instance->RTTI_GetType()] = instance;
            }

            componentEditor->AddInstance(instance, aggregateInstance, nullptr);

            // Try and get the underlying SC entity
            AZStd::any* userData{};
            GraphCanvas::NodeRequestBus::EventResult(userData, instance->GetEntityId(), &GraphCanvas::NodeRequests::GetUserData);
            AZ::EntityId scriptCanvasId = userData && userData->is<AZ::EntityId>() ? *AZStd::any_cast<AZ::EntityId>(userData) : AZ::EntityId();
            if (scriptCanvasId.IsValid())
            {
                entitySet.insert(scriptCanvasId);
            }
            else
            {
                entitySet.insert(instance->GetEntityId());
            }
        }
    }

    const AZStd::string GetMethod(AZ::Component* component)
    {
        auto classData = AzToolsFramework::GetComponentClassData(component);
        if (!classData)
        {
            return "";
        }

        if (!classData->m_azRtti->IsTypeOf<ScriptCanvas::Nodes::Core::Method>())
        {
            return "";
        }

        ScriptCanvas::Nodes::Core::Method* method = azrtti_cast<ScriptCanvas::Nodes::Core::Method*>(component);
        return method->GetMethodClassName() + method->GetName();
    }

    AZStd::string GetEBusEventHandlerString(const AZ::EntityId& entityId,
                                            AZ::Component* component)
    {
        auto classData = AzToolsFramework::GetComponentClassData(component);
        if (!classData)
        {
            return "";
        }

        if (!classData->m_azRtti->IsTypeOf<ScriptCanvas::Nodes::Core::EBusEventHandler>())
        {
            return "";
        }

        ScriptCanvas::Nodes::Core::EBusEventHandler* eventHandler = azrtti_cast<ScriptCanvas::Nodes::Core::EBusEventHandler*>(component);

        // IMPORTANT: A wrapped node will have an event name. NOT a wrapper node.
        AZStd::string eventName;
        ScriptCanvasEditor::EBusHandlerEventNodeDescriptorRequestBus::EventResult(eventName, entityId, &ScriptCanvasEditor::EBusHandlerEventNodeDescriptorRequests::GetEventName);

        AZStd::string result = eventHandler->GetEBusName() + eventName;
        return result;
    }

    // Returns a set of unique display component instances
    AZStd::list<AZ::Component*> GetVisibleGcInstances(const AZ::EntityId& entityId)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        AZStd::list<AZ::Component*> result;

        GraphCanvas::GraphCanvasPropertyBus::EnumerateHandlersId(entityId,
            [&result](GraphCanvas::GraphCanvasPropertyInterface* propertyInterface) -> bool
            {
                AZ::Component* component = propertyInterface->GetPropertyComponent();
                if (AzToolsFramework::ShouldInspectorShowComponent(component))
                {
                    result.push_back(component);
                }

                // Continue enumeration.
                return true;
            });

        return result;
    }

    // Returns a set of unique display component instances
    AZStd::list<AZ::Component*> GetVisibleScInstances(const AZ::EntityId& entityId)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        // GraphCanvas entityId -> scriptCanvasEntity
        AZStd::any* userData {};
        GraphCanvas::NodeRequestBus::EventResult(userData, entityId, &GraphCanvas::NodeRequests::GetUserData);
        AZ::EntityId scriptCanvasId = userData && userData->is<AZ::EntityId>() ? *AZStd::any_cast<AZ::EntityId>(userData) : AZ::EntityId();
        if (!scriptCanvasId.IsValid())
        {
            return AZStd::list<AZ::Component*>();
        }

        AZ::Entity* scriptCanvasEntity = AzToolsFramework::GetEntityById(scriptCanvasId);
        if (!scriptCanvasEntity)
        {
            return AZStd::list<AZ::Component*>();
        }

        // scriptCanvasEntity -> ScriptCanvas::Node
        AZStd::list<AZ::Component*> result;
        auto components = AZ::EntityUtils::FindDerivedComponents<ScriptCanvas::Node>(scriptCanvasEntity);
        for (auto component : components)
        {
            if (AzToolsFramework::ShouldInspectorShowComponent(component))
            {
                result.push_back(component);
            }
        }

        return result;
    }

    void MoveInstances(const AZStd::string& position,
        const AZ::EntityId& entityId,
        AZStd::list<AZ::Component*>& gcInstances,
        AZStd::list<AZ::Component*>& scInstances,
        StringToInstanceMap& instancesToDisplay)
    {
        GRAPH_CANVAS_PROFILE_FUNCTION();
        if (position.empty() ||
            (gcInstances.empty() && scInstances.empty()))
        {
            return;
        }

        auto& entry = instancesToDisplay[position];
        if (!entry.m_gcEntityId.IsValid())
        {
            entry.m_gcEntityId = entityId;
        }

        if (!gcInstances.empty())
        {
            entry.m_gcInstances.splice(entry.m_gcInstances.end(), gcInstances);
        }

        if (!scInstances.empty())
        {
            entry.m_scInstances.splice(entry.m_scInstances.end(), scInstances);
        }
    }

    AZStd::string GetKeyForInstancesToDisplay(const AZ::EntityId& entityId,
        const AZStd::list<AZ::Component*>& gcInstances,
        const AZStd::list<AZ::Component*>& scInstances)
    {
        GRAPH_CANVAS_PROFILE_FUNCTION();
        AZStd::string result;

        if (!scInstances.empty())
        {
            auto component = scInstances.front();

            result = GetMethod(component);
            if (!result.empty())
            {
                return result;
            }

            result = GetEBusEventHandlerString(entityId, component);
            if (!result.empty())
            {
                return result;
            }

            return component->RTTI_GetType().ToString<AZStd::string>();
        }

        if (!gcInstances.empty())
        {
            return gcInstances.front()->RTTI_GetType().ToString<AZStd::string>();
        }

        return result;
    }

    void GetInstancesToDisplay(const AZStd::vector<AZ::EntityId>& selectedEntityIds,
        StringToInstanceMap& instancesToDisplay)
    {
        GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
        for (auto& entityId : selectedEntityIds)
        {
            GRAPH_CANVAS_DETAILED_PROFILE_SCOPE("GetInstancesToDisplay::InnerLoop");
            AZStd::list<AZ::Component*> gcInstances = GetVisibleGcInstances(entityId);
            AZStd::list<AZ::Component*> scInstances = GetVisibleScInstances(entityId);

            AZStd::string position = GetKeyForInstancesToDisplay(entityId, gcInstances, scInstances);

            MoveInstances(position, entityId, gcInstances, scInstances, instancesToDisplay);
        }
    }
}

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        PropertyGrid::PropertyGrid(QWidget* parent /*= nullptr*/, const char* name /*= "Properties"*/)
            : AzQtComponents::StyledDockWidget(parent)
        {
            // This is used for styling.
            setObjectName("PropertyGrid");

            m_spacer = new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Expanding);

            setWindowTitle(name);
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

            m_scrollArea = new QScrollArea(this);
            m_scrollArea->setWidgetResizable(true);
            m_scrollArea->setSizePolicy(QSizePolicy::Policy::Ignored, QSizePolicy::Policy::Ignored);

            m_host = new QWidget;
            m_host->setLayout(new QVBoxLayout());

            m_scrollArea->setWidget(m_host);
            setWidget(m_scrollArea);

            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZ_Assert(serializeContext, "Failed to acquire application serialize context.");

            UpdateContents(AZStd::vector<AZ::EntityId>());

            PropertyGridRequestBus::Handler::BusConnect();
        }

        PropertyGrid::~PropertyGrid()
        {
        }

        void PropertyGrid::ClearSelection()
        {
            GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
            for (auto& componentEditor : m_componentEditors)
            {
                // Component editor deletion needs to be deferred until the next frame
                // as ClearSelection can be called when a slot is removed via the Reflected
                // therefore causing the reflected property editor to be deleted while it is still
                // in the callstack
                // Deleting a node will cause the selection change event to be fired from the GraphCanvas Scene which leads to the selection being cleared
                // Furthermore that change queues a property editor refresh for next frame, which if the node contained an EntityId slot it attempts to access
                // the node address which has been deleted.
                // Therefore the property editor property modification refresh level is set to none to prevent a refresh before it gets deleted
                componentEditor->GetPropertyEditor()->CancelQueuedRefresh();
                componentEditor->setVisible(false);
                componentEditor.release()->deleteLater();
            }

            m_componentEditors.clear();

            ScriptCanvas::EndpointNotificationBus::MultiHandler::BusDisconnect();
            ScriptCanvas::NodeNotificationsBus::MultiHandler::BusDisconnect();

            GraphCanvas::GraphCanvasPropertyInterfaceNotificationBus::MultiHandler::BusDisconnect();
        }

        void PropertyGrid::OnPropertyComponentChanged()
        {
            RefreshPropertyGrid();
        }

        void PropertyGrid::DisplayInstances(const InstancesToDisplay& instances)
        {
            GRAPH_CANVAS_PROFILE_FUNCTION();
            if (instances.m_gcInstances.empty() &&
                instances.m_scInstances.empty())
            {
                return;
            }

            AzToolsFramework::ComponentEditor* componentEditor = CreateComponentEditor();

            AZ::Component* firstGcInstance = !instances.m_gcInstances.empty() ? instances.m_gcInstances.front() : nullptr;
            AZ::Component* firstScInstance = !instances.m_scInstances.empty() ? instances.m_scInstances.front() : nullptr;

            AZStd::unordered_map<AZ::TypeId, AZ::Component*> firstOfTypeMap;
            AZStd::unordered_set<AZ::EntityId> entitySet;

            // This adds all the component instances to the component editor widget and aggregates them based on the component types
            AddInstancesToComponentEditor(componentEditor, instances.m_gcInstances, firstOfTypeMap, entitySet);
            AddInstancesToComponentEditor(componentEditor, instances.m_scInstances, firstOfTypeMap, entitySet);

            // Set the title.
            // This MUST be done AFTER AddInstance() to override the default title.
            AZStd::string title = GetTitle(instances.m_gcEntityId, firstScInstance ? firstScInstance : firstGcInstance);

            // Use the number of unique entities to determine the number of selected entities for this component editor
            if (entitySet.size() > 1)
            {
                title += AZStd::string::format(" (%zu Selected)", entitySet.size());
            }

            componentEditor->GetHeader()->SetTitle(title.c_str());

            {
                GRAPH_CANVAS_DETAILED_PROFILE_SCOPE("PropertyGrid::DisplayInstance::RefreshEditor");

                // Refresh editor
                componentEditor->AddNotifications();
                componentEditor->SetExpanded(true);
                componentEditor->InvalidateAll();
            }

            // hiding the icon on the header for Preview
            componentEditor->GetHeader()->SetIcon(QIcon());

            componentEditor->show();
        }

        ScriptCanvas::ScriptCanvasId PropertyGrid::GetScriptCanvasId(AZ::Component* component)
        {
            ScriptCanvas::ScriptCanvasId executionId;
            if (const ScriptCanvas::Node* node = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Node>(component->GetEntity()))
            {
                executionId = node->GetOwningScriptCanvasId();
            }
            else
            {
                GeneralRequestBus::BroadcastResult(executionId, &GeneralRequests::GetActiveScriptCanvasId);

                if (!executionId.IsValid())
                {
                    AZ::EntityId graphCanvasGraphId;

                    // GraphCanvas Node
                    GraphCanvas::SceneMemberRequestBus::EventResult(graphCanvasGraphId, component->GetEntityId(), &GraphCanvas::SceneMemberRequests::GetScene);
                    GeneralRequestBus::BroadcastResult(executionId, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);
                }
            }

            return executionId;
        }

        AzToolsFramework::ComponentEditor* PropertyGrid::CreateComponentEditor()
        {
            GRAPH_CANVAS_PROFILE_FUNCTION();
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZ_Assert(serializeContext, "Failed to acquire application serialize context.");

            {
                GRAPH_CANVAS_DETAILED_PROFILE_SCOPE("CreateComponentEditor::ComponentConstruction");
                m_componentEditors.push_back(AZStd::make_unique<AzToolsFramework::ComponentEditor>(serializeContext, this, this));
            }

            AzToolsFramework::ComponentEditor* componentEditor = m_componentEditors.back().get();

            {
                GRAPH_CANVAS_DETAILED_PROFILE_SCOPE("CreateComponentEditor::ComponentConfiguration");

                componentEditor->GetHeader()->SetHasContextMenu(false);
                componentEditor->GetPropertyEditor()->SetHideRootProperties(false);
                componentEditor->GetPropertyEditor()->SetAutoResizeLabels(true);

                connect(componentEditor, &AzToolsFramework::ComponentEditor::OnExpansionContractionDone, this, [this]()
                {
                    m_host->layout()->update();
                    m_host->layout()->activate();
                });
            }

            {
                GRAPH_CANVAS_DETAILED_PROFILE_SCOPE("CreateComponentEditor::SpacerUpdates");
                //move spacer to bottom of editors
                m_host->layout()->removeItem(m_spacer);
                m_host->layout()->addWidget(componentEditor);
                m_host->layout()->addItem(m_spacer);
                m_host->layout()->update();

            }

            return componentEditor;
        }

        void PropertyGrid::SetSelection(const AZStd::vector<AZ::EntityId>& selectedEntityIds)
        {
            GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
            ClearSelection();

            for (const AZ::EntityId& gcEntityId : selectedEntityIds)
            {
                GraphCanvas::GraphCanvasPropertyInterfaceNotificationBus::MultiHandler::BusConnect(gcEntityId);
            }

            UpdateContents(selectedEntityIds);
            RefreshPropertyGrid();
        }

        void PropertyGrid::OnNodeUpdate(const AZ::EntityId&)
        {
            RefreshPropertyGrid();
        }

        void PropertyGrid::DisableGrid()
        {
            setEnabled(false);

            for (auto& componentEditor : m_componentEditors)
            {
                componentEditor->setEnabled(false);
            }
        }

        void PropertyGrid::EnableGrid()
        {
            setEnabled(true);

            for (auto& componentEditor : m_componentEditors)
            {
                componentEditor->setEnabled(true);
            }
        }

        void PropertyGrid::BeforePropertyModified(AzToolsFramework::InstanceDataNode* pNode)
        {
            // For strings we want to signal out once we are finished editing the string. Mainly to help deal with
            // issues where the string contorls the layout of hte node(ala print/build string nodes).
            //
            // But the SetPropertyEditingActive signal doesn't seem to be hooked up to anything, so can't generically wrap this.
            // Instead we will push an extra 'undo' when we are going into a string modify, mark ourselves as 'dirty'
            // then pop as normal in the after, then signal out the undo once we are finished editing.
            if (pNode->GetElementMetadata()->m_typeId == azrtti_typeid<AZStd::string>()
                || pNode->GetElementMetadata()->m_typeId == azrtti_typeid<AZ::Color>())
            {
                if (!m_propertyModified)
                {
                    m_propertyModified = true;
                    GeneralRequestBus::Broadcast(&GeneralRequests::PushPreventUndoStateUpdate);
                }
            }

            GeneralRequestBus::Broadcast(&GeneralRequests::PushPreventUndoStateUpdate);
        }

        void PropertyGrid::AfterPropertyModified(AzToolsFramework::InstanceDataNode* pNode)
        {
            if (pNode->GetElementMetadata()->m_typeId == azrtti_typeid<AZStd::string>()
                || pNode->GetElementMetadata()->m_typeId == azrtti_typeid<AZ::Color>())
            {
                GeneralRequestBus::Broadcast(&GeneralRequests::PopPreventUndoStateUpdate);
            }
            else
            {
                SignalUndo(pNode);
            }
        }

        void PropertyGrid::SetPropertyEditingActive(AzToolsFramework::InstanceDataNode*)
        {
            // This signal doesn't actually get called.
        }

        void PropertyGrid::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* pNode)
        {
            if (pNode->GetElementMetadata()->m_typeId == azrtti_typeid<AZStd::string>()
                || pNode->GetElementMetadata()->m_typeId == azrtti_typeid<AZ::Color>())
            {
                if (m_propertyModified)
                {
                    m_propertyModified = false;
                    SignalUndo(pNode);
                }
            }
        }

        void PropertyGrid::RequestPropertyContextMenu(AzToolsFramework::InstanceDataNode* node, const QPoint& point)
        {
            PropertyGridContextMenu contextMenu(node);
            if (!contextMenu.actions().empty())
            {
                contextMenu.exec(point);
            }
        }

        void PropertyGrid::OnSlotDisplayTypeChanged(const ScriptCanvas::SlotId& slotId, const ScriptCanvas::Data::Type&)
        {
            const AZ::EntityId* nodeId = ScriptCanvas::NodeNotificationsBus::GetCurrentBusId();

            if (nodeId)
            {
                ScriptCanvas::Endpoint scriptCanvasEndpoint((*nodeId), slotId);
                UpdateEndpointVisibility(scriptCanvasEndpoint);
            }
        }

        void PropertyGrid::RefreshPropertyGrid()
        {
            GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
            for (auto& componentEditor : m_componentEditors)
            {
                if (componentEditor->isVisible())
                {
                    componentEditor->QueuePropertyEditorInvalidation(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_Values);
                }
                else
                {
                    break;
                }
            }
        }
        
        void PropertyGrid::RebuildPropertyGrid()
        {
            for (auto& componentEditor : m_componentEditors)
            {
                if (componentEditor->isVisible())
                {
                    componentEditor->QueuePropertyEditorInvalidation(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_EntireTree);
                }
                else
                {
                    break;
                }
            }
        }

        void PropertyGrid::SetVisibility(const AZStd::vector<AZ::EntityId>& selectedEntityIds)
        {
            GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();

            // Set the visibility and connect for changes.
            for (auto& gcNodeEntityId : selectedEntityIds)
            {
                // GC node -> SC node.
                AZStd::any* nodeUserData = nullptr;
                GraphCanvas::NodeRequestBus::EventResult(nodeUserData, gcNodeEntityId, &GraphCanvas::NodeRequests::GetUserData);
                AZ::EntityId scNodeEntityId = nodeUserData && nodeUserData->is<AZ::EntityId>() ? *AZStd::any_cast<AZ::EntityId>(nodeUserData) : AZ::EntityId();

                AZ::Entity* nodeEntity{};
                AZ::ComponentApplicationBus::BroadcastResult(nodeEntity, &AZ::ComponentApplicationRequests::FindEntity, scNodeEntityId);
                auto node = nodeEntity ? AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Node>(nodeEntity) : nullptr;
                if (!node)
                {
                    continue;
                }

                ScriptCanvas::NodeNotificationsBus::MultiHandler::BusConnect(node->GetEntityId());

                AZStd::vector<AZ::EntityId> gcSlotEntityIds;
                GraphCanvas::NodeRequestBus::EventResult(gcSlotEntityIds, gcNodeEntityId, &GraphCanvas::NodeRequests::GetSlotIds);

                for (auto& gcSlotEntityId : gcSlotEntityIds)
                {
                    // GC slot -> SC slot.
                    AZStd::any* slotUserData = nullptr;
                    GraphCanvas::SlotRequestBus::EventResult(slotUserData, gcSlotEntityId, &GraphCanvas::SlotRequests::GetUserData);
                    ScriptCanvas::SlotId scSlotId = slotUserData && slotUserData->is<ScriptCanvas::SlotId>() ? *AZStd::any_cast<ScriptCanvas::SlotId>(slotUserData) : ScriptCanvas::SlotId();

                    ScriptCanvas::Slot* slot = node->GetSlot(scSlotId);

                    if (!slot || slot->GetDescriptor() != ScriptCanvas::SlotDescriptors::DataIn())
                    {
                        continue;
                    }

                    slot->UpdateDatumVisibility();

                    // Connect to get notified of changes.
                    ScriptCanvas::EndpointNotificationBus::MultiHandler::BusConnect(ScriptCanvas::Endpoint(scNodeEntityId, scSlotId));
                }
            }
        }

        void PropertyGrid::UpdateContents(const AZStd::vector<AZ::EntityId>& selectedEntityIds)
        {
            GRAPH_CANVAS_DETAILED_PROFILE_FUNCTION();
            if (!selectedEntityIds.empty())
            {
                // Build up components to display
                StringToInstanceMap instanceMap;
                GetInstancesToDisplay(selectedEntityIds, instanceMap);

                SetVisibility(selectedEntityIds);

                for (auto& pair : instanceMap)
                {
                    GRAPH_CANVAS_DETAILED_PROFILE_SCOPE("PropertyGrid::UpdateContents::InstanceMapLoop");
                    DisplayInstances(pair.second);
                }
            }
        }

        void PropertyGrid::OnEndpointConnected(const ScriptCanvas::Endpoint& )
        {
            const ScriptCanvas::Endpoint* sourceEndpoint = ScriptCanvas::EndpointNotificationBus::GetCurrentBusId();

            if (sourceEndpoint)
            {
                UpdateEndpointVisibility(*sourceEndpoint);
            }
        }

        void PropertyGrid::OnEndpointDisconnected(const ScriptCanvas::Endpoint& )
        {
            const ScriptCanvas::Endpoint* sourceEndpoint = ScriptCanvas::EndpointNotificationBus::GetCurrentBusId();

            if (sourceEndpoint)
            {
                UpdateEndpointVisibility(*sourceEndpoint);
            }
        }

        void PropertyGrid::OnEndpointConvertedToValue()
        {
            const ScriptCanvas::Endpoint* sourceEndpoint = ScriptCanvas::EndpointNotificationBus::GetCurrentBusId();

            if (sourceEndpoint)
            {
                UpdateEndpointVisibility(*sourceEndpoint);
            }
        }

        void PropertyGrid::OnEndpointConvertedToReference()
        {
            const ScriptCanvas::Endpoint* sourceEndpoint = ScriptCanvas::EndpointNotificationBus::GetCurrentBusId();

            if (sourceEndpoint)
            {
                UpdateEndpointVisibility(*sourceEndpoint);
            }
        }

        void PropertyGrid::UpdateEndpointVisibility(const ScriptCanvas::Endpoint& endpoint)
        {
            ScriptCanvas::Slot* slot = nullptr;
            ScriptCanvas::NodeRequestBus::EventResult(slot, endpoint.GetNodeId(), &ScriptCanvas::NodeRequests::GetSlot, endpoint.GetSlotId());

            if (slot)
            {
                slot->UpdateDatumVisibility();
                RebuildPropertyGrid();
            }
        }

        void PropertyGrid::SignalUndo(AzToolsFramework::InstanceDataNode* pNode)
        {
            GeneralRequestBus::Broadcast(&GeneralRequests::PopPreventUndoStateUpdate);

            AzToolsFramework::InstanceDataNode* componentNode = pNode;
            do
            {
                auto* componentClassData = componentNode->GetClassMetadata();
                if (componentClassData && componentClassData->m_azRtti && componentClassData->m_azRtti->IsTypeOf(azrtti_typeid<AZ::Component>()))
                {
                    break;
                }
                componentNode = componentNode->GetParent();
            }
            while (componentNode);

            if (!componentNode)
            {
                AZ_Warning("Script Canvas", false, "Failed to locate component data associated with the script canvas property. Unable to mark parent Entity as dirty.");
                return;
            }

            // Only need one instance to lookup the SceneId in-order to record the undo state
            const size_t firstInstanceIdx = 0;
            if (componentNode->GetNumInstances())
            {
                AZ::SerializeContext* context = componentNode->GetSerializeContext();
                AZ::Component* componentInstance = context->Cast<AZ::Component*>(componentNode->GetInstance(firstInstanceIdx), componentNode->GetClassMetadata()->m_typeId);
                if (componentInstance && componentInstance->GetEntity())
                {
                    ScriptCanvas::ScriptCanvasId scriptCanvasId = GetScriptCanvasId(componentInstance);
                    GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, scriptCanvasId);
                }
            }
        }

#include <Editor/View/Widgets/moc_PropertyGrid.cpp>
    }
}
