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

namespace AZ::DocumentPropertyEditor
{
    class AdapterBuilder;

    //! ComponentAdapter is responsible to listening for signals that affect each component in the Entity Inspector
    class ComponentAdapter
        : public ReflectionAdapter
        , private AzToolsFramework::PropertyEditorEntityChangeNotificationBus::MultiHandler
        , private AzToolsFramework::ToolsApplicationEvents::Bus::Handler
        , private AzToolsFramework::PropertyEditorGUIMessages::Bus::Handler
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

        // AzToolsFramework::PropertyEditorGUIMessages::Bus overrides
        void RequestRefresh(AzToolsFramework::PropertyModificationRefreshLevel level) override;

        //! Sets the component, connects the appropriate Bus Handlers and sets the reflect data for this instance
        void SetComponent(AZ::Component* componentInstance);

        //! Trigger a refresh based on messages from the listeners
        void DoRefresh();

        Dom::Value HandleMessage(const AdapterMessage& message) override;

        //! Request the PrefabAdapterInterface to add a property handler if an override is present corresponding to the path provided.
        //! @param adapterBuilder The adapter builder to use for adding property handler
        //! @param serializedpath The serialized path to use to check whether an override is present corresponding to it
        void OnBeginRow(AdapterBuilder* adapterBuilder, AZStd::string_view serializedPath) override;

    protected:

        AZStd::string m_componentAlias;
        AZ::EntityId m_entityId;

        AZ::Component* m_componentInstance = nullptr;

        AzToolsFramework::UndoSystem::URSequencePoint* m_currentUndoNode = nullptr;

        enum AzToolsFramework::PropertyModificationRefreshLevel m_queuedRefreshLevel =
            AzToolsFramework::PropertyModificationRefreshLevel::Refresh_None;
    };

} // namespace AZ::DocumentPropertyEditor
