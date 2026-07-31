/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/Backends/JSON/JsonSerializationUtils.h>
#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/DPEComponentAdapter.h>
#include <AzToolsFramework/Prefab/DocumentPropertyEditor/PrefabAdapterInterface.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabFocusPublicInterface.h>
#include <QTimer>
#include <AzToolsFramework/API/ToolsApplicationAPI.h> // scoped undo batch

namespace AZ::DocumentPropertyEditor
{
    ComponentAdapter::ComponentAdapter() = default;

    ComponentAdapter::ComponentAdapter(AZ::Component* componentInstace)
    {
        SetComponent(componentInstace);
    }

    ComponentAdapter::~ComponentAdapter()
    {
        AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::BusDisconnect();
        AzToolsFramework::ToolsApplicationEvents::Bus::Handler::BusDisconnect();
        if (m_entityId.IsValid())
        {
            AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler::BusDisconnect(m_entityId);
        }
    }

    void ComponentAdapter::OnEntityComponentPropertyChanged(AZ::ComponentId componentId)
    {
        if (m_componentId == componentId)
        {
            RequestRefresh(AzToolsFramework::PropertyModificationRefreshLevel::Refresh_Values);
        }
    }

    void ComponentAdapter::InvalidatePropertyDisplay(AzToolsFramework::PropertyModificationRefreshLevel level)
    {
        RequestRefresh(level);
    }

    void ComponentAdapter::InvalidatePropertyDisplayForComponent(AZ::EntityComponentIdPair entityComponentIdPair, AzToolsFramework::PropertyModificationRefreshLevel level)
    {
        if ((entityComponentIdPair.GetEntityId() == m_entityId) && (entityComponentIdPair.GetComponentId() == m_componentId))
        {
            RequestRefresh(level);
        }
    }

    void ComponentAdapter::RequestRefresh(AzToolsFramework::PropertyModificationRefreshLevel level)
    {
        if (level > m_queuedRefreshLevel)
        {
            if (m_queuedRefreshLevel == AzToolsFramework::PropertyModificationRefreshLevel::Refresh_None)
            {
                QPointer<QObject> stillAlive(&m_stillAlive);
                QTimer::singleShot(
                    0,
                    [this, stillAlive]()
                    {
                        // make sure the component adapter still exists by the time this refresh resolves
                        if (stillAlive)
                        {
                            DoRefresh();
                        }
                    });
            }
            m_queuedRefreshLevel = level;
        }
    }

    void ComponentAdapter::SetComponent(AZ::Component* componentInstance)
    {
        if (!componentInstance) // This happens if the entity we're attached to is destroyed and becomes invalid.
        {
            ClearValue();
            return;
        }

        AZ::EntityId newEntityId = componentInstance->GetEntityId();

        bool reconnectToSpecificEntityBus = false;
        if (m_entityId != newEntityId)
        {
            if (m_entityId.IsValid())
            {
                AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler::BusDisconnect(m_entityId);
            }
            reconnectToSpecificEntityBus = true;
        }

        m_entityId = newEntityId;
        m_componentId = componentInstance->GetId();

        if (!AZ::EntitySystemBus::Handler::BusIsConnected())
        {
            AZ::EntitySystemBus::Handler::BusConnect(); // listens for "On Entity Destruction" / "On Entity Initialized".
        }

        if (!AzToolsFramework::ToolsApplicationEvents::Bus::Handler::BusIsConnected())
        {
            AzToolsFramework::ToolsApplicationEvents::Bus::Handler::BusConnect();
        }

        if (!AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::BusIsConnected())
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::BusConnect();
        }

        if (reconnectToSpecificEntityBus)
        {
            AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler::BusConnect(m_entityId);
        }
        
        AZ::Uuid instanceTypeId = azrtti_typeid(componentInstance);
        SetValue(componentInstance, instanceTypeId);
    }

    AZ::Component* ComponentAdapter::GetComponentInstanceFromId() const
    {
        if (m_entityId.IsValid())
        {
            const Entity* entity = AzToolsFramework::GetEntity(m_entityId);

            // Since DoRefresh() gets called on the next tick, the entity and its components could have been destroyed by then.
            if (entity == nullptr)
            {
                return nullptr;
            }

            return entity->FindComponent(m_componentId);
        }

        return nullptr;
    }

    bool ComponentAdapter::IsComponentValid() const
    {
        AZ::Component* component = GetComponentInstanceFromId();
        if (component)
        {
            return component->GetEntity()->GetState() == AZ::Entity::State::Active;
        }
        return false;
    }

    void ComponentAdapter::DoRefresh()
    {
        if (IsComponentValid())
        {
            m_queuedRefreshLevel = AzToolsFramework::PropertyModificationRefreshLevel::Refresh_None;
            QueueResetDocument();
        }
    }

    Dom::Value ComponentAdapter::HandleMessage(const AdapterMessage& message)
    {
        auto handlePropertyEditorChanged = [&]([[maybe_unused]] const Dom::Value& valueFromEditor, Nodes::ValueChangeType changeType)
        {
            if (!IsComponentValid())
            {
                return;
            }

            switch (changeType)
            {
            case Nodes::ValueChangeType::InProgressEdit:
                m_gotInProgressEdit = true;

                // At this point,  we expect the value to have already been modified in the entity.  In the case of reflected
                // properties, the RPEPropertyHandlerWrapper<T> class has already validated and then written the value into
                // the underlying entity component object, before it invokes this message.  In the case of the container buttons
                // that add/remove container elements, also, this custom handler has already modified the underlying container,
                // before it invokes this message.
                // If you make a custom handler, make sure that you either mutate the underlying data first before invoking this
                // event, or capture a custom undo yourself.
                if (m_entityId.IsValid())
                {
                    AzToolsFramework::ScopedUndoBatch batch("Modify Component Property", &m_currentUndoBatch);
                    batch.MarkEntityDirty(m_entityId);
                }
                break;
            case Nodes::ValueChangeType::FinishedEdit:
                // if you find yourself here and the assert triggers, it means someone created some sort of widget or control
                // that sends only a FinishedEdit, without an InProgressEdit being emitted beforehand.
                // Controls that have continuous changes like sliders or text boxes (each keystroke) need to issue a
                // InProgressEdit for each change, then a FinishedEdit when the user is done (arbitrary call to make, could
                // be something like lost focus, could be hitting return, could be releasing a mouse button).
                // Instantanous controls like buttons, checkboxes, etc, should issue both an InProgressEdit and FinishedEdit
                // in the same call, since they are effectively atomic.  Doing so allows this code to deal with the undo
                // stack up front and not have to do undo/redo capture operations at the end of the change when editing is finished.

                // Note that if you are deep in the UI code for a component or writing a Handler, this means issuing:
                // AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::RequestWrite, gui);
                // AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::OnEditingFinished, gui);
                // since those result in InProgressEdit, FinishedEdit being sent to the ComponentAdapter.
                AZ_Assert(m_gotInProgressEdit, "ComponentAdapter::HandleMessage - Got FinishedEdit without InProgressEdit.");
                m_gotInProgressEdit = false;
                m_currentUndoBatch = nullptr;
                break;
            }
        };

        Dom::Value returnValue = message.Match(Nodes::PropertyEditor::OnChanged, handlePropertyEditorChanged);

        ReflectionAdapter::HandleMessage(message);

        return returnValue;
    }

    void ComponentAdapter::CreateLabel(AdapterBuilder* adapterBuilder, AZStd::string_view labelText, AZStd::string_view serializedPath)
    {
        ReflectionAdapter::CreateLabel(adapterBuilder, labelText, serializedPath);
    }

    void ComponentAdapter::OnEntityDeactivated(const AZ::EntityId& entityId)
    {
        // There is no "Component is destroyed" event, so we have to listen for the entity deactivation
        // and make no assumptions from that point until we get our entity back.
        if (entityId == m_entityId)
        {
            // stop listening for all events except Entity Initialized, which will be reconnected when the entity is re-initialized
            // in case its an undo / redo op.
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::BusDisconnect();
            AzToolsFramework::ToolsApplicationEvents::Bus::Handler::BusDisconnect();
            AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler::BusDisconnect(m_entityId);
            SetComponent(nullptr);
        }
    }

    void ComponentAdapter::OnEntityActivated(const AZ::EntityId& entityId)
    {
        if (entityId == m_entityId)
        {
            // reconnect to the various busses as our entity has been restored.
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler::BusConnect();
            AzToolsFramework::ToolsApplicationEvents::Bus::Handler::BusConnect();
            AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler::BusConnect(m_entityId);

            // We can't assume that its the same component, even if the component id AND memory is the same.
            // The component could have been destroyed and recreated, so we need to re-fetch the component instance from the entity.
            SetComponent(GetComponentInstanceFromId());
        }
    }

} // namespace AZ::DocumentPropertyEditor
