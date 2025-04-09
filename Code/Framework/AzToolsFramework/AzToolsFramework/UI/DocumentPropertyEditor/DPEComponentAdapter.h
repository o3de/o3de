/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzFramework/DocumentPropertyEditor/ReflectionAdapter.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzCore/Component/EntityBus.h>
#include <QObject>

namespace AZ::DocumentPropertyEditor
{
    class AdapterBuilder;

    //! ComponentAdapter is responsible to listening for signals that affect each component in the Entity Inspector
    class ComponentAdapter
        : public ReflectionAdapter
        , private AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler
        , private AzToolsFramework::ToolsApplicationEvents::Bus::Handler
        , private AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler
        , private AZ::EntitySystemBus::Handler
    {
    public:
        //! Creates an uninitialized (empty) ComponentAdapter.
        ComponentAdapter();
        //! Creates a ComponentAdapter with a specific component instance, see SetComponent
        ComponentAdapter(AZ::Component* componentInstance);
        ~ComponentAdapter();

        // AzToolsFramework::PropertyEditorEntityChangeNotificationBus::Bus overrides
        void OnEntityComponentPropertyChanged(AZ::ComponentId componentId) override;

        // AzToolsFramework::ToolsApplicationEvents::Bus overrides
        void InvalidatePropertyDisplay(AzToolsFramework::PropertyModificationRefreshLevel level) override;

        // AzToolsFramework::ToolsApplicationEvents::Bus overrides
        void InvalidatePropertyDisplayForComponent(AZ::EntityComponentIdPair entityComponentIdPair, AzToolsFramework::PropertyModificationRefreshLevel level) override;

        // AzToolsFramework::PropertyEditorGUIMessages::Bus overrides
        void RequestRefresh(AzToolsFramework::PropertyModificationRefreshLevel level) override;

        //! Sets the component, connects the appropriate Bus Handlers and sets the reflect data for this instance
        virtual void SetComponent(AZ::Component* componentInstance);

        Dom::Value HandleMessage(const AdapterMessage& message) override;

        //! Creates a node for displaying label information.
        //! Requests the PrefabAdapterInterface to add a property node and configures its style if an override exist.
        //! @param adapterBuilder The adapter builder to use for adding property node.
        //! @param labelText The text string to be displayed in label.
        //! @param serializedPath The serialized path to use to check whether an override is present corresponding to it.
        void CreateLabel(AdapterBuilder* adapterBuilder, AZStd::string_view labelText, AZStd::string_view serializedPath) override;

        //! Gets notification from the EntitySystemBus before destroying an entity.
        void OnEntityDestruction(const AZ::EntityId&) override;

    private:
        //! Checks if the component is still valid in the entity.
        bool IsComponentValid() const;
        void DoRefresh();

    protected:
        AZ::EntityId m_entityId;

        // Should call IsComponentValid() for validity check before using the component id and its component instance.
        AZ::ComponentId m_componentId = AZ::InvalidComponentId;

        AzToolsFramework::UndoSystem::URSequencePoint* m_currentUndoBatch = nullptr;

        enum AzToolsFramework::PropertyModificationRefreshLevel m_queuedRefreshLevel =
            AzToolsFramework::PropertyModificationRefreshLevel::Refresh_None;

        //! object, used in conjunction with a QPointer, to track if this component is still alive
        QObject m_stillAlive;
    };

} // namespace AZ::DocumentPropertyEditor
