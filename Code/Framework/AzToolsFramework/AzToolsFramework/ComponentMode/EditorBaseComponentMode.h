/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <AzToolsFramework/Viewport/ActionBus.h>

namespace AzToolsFramework
{
    namespace ComponentModeFramework
    {
         /// Abstract class to be inherited from by concrete ComponentModes.
         /// Exposes ComponentMode interface and handles some useful common
         /// functionality all ComponentModes require.
        class EditorBaseComponentMode
            : public ComponentModeRequestBus::Handler
            , protected ToolsApplicationNotificationBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR_DECL

            /// @cond
            EditorBaseComponentMode(
                const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType);
            EditorBaseComponentMode(EditorBaseComponentMode&&) = default;
            EditorBaseComponentMode& operator=(EditorBaseComponentMode&&) = default;
            virtual ~EditorBaseComponentMode();
            /// @endcond

            /// ComponentMode interface - populate actions for this ComponentMode.
            /// When PopulateActions is called, if a second action override is found with the
            /// same key, it should override the existing action if one already exists.
            /// (e.g. The 'escape' key will first deselect a vertex, then leave ComponentMode if
            /// an action is added to deselect a vertex when one is selected)
            /// @attention More specific actions come later in the ordering when they are added.
            AZStd::vector<ActionOverride> PopulateActions() final;

            /// Populate the Viewport UI widget for this ComponentMode.
            AZStd::vector<ViewportUi::ClusterId> PopulateViewportUi() final;

            /// ComponentModeRequestBus ...
            void PostHandleMouseInteraction() final;

        protected:
            /// The EntityId this ComponentMode instance is associated with.
            AZ::EntityId GetEntityId() const { return m_entityComponentIdPair.GetEntityId(); }
            /// The combined Entity and Component Id to uniquely identify a specific Component on a given Entity.
            /// Note: This is required when more than one Component of the same type can exists on an Entity at a time.
            AZ::EntityComponentIdPair GetEntityComponentIdPair() const { return m_entityComponentIdPair; }

            /// The ComponentId this ComponentMode instance is associated with.
            AZ::ComponentId GetComponentId() const final { return m_entityComponentIdPair.GetComponentId(); }
            /// The underlying Component type for this ComponentMode.
            AZ::Uuid GetComponentType() const final { return m_componentType; }

            /// EditorBaseComponentMode interface
            /// @see To be overridden by derived ComponentModes
            virtual AZStd::vector<ViewportUi::ClusterId> PopulateViewportUiImpl();

        private:
            /// EditorBaseComponentMode interface
            /// @see To be overridden by derived ComponentModes
            virtual AZStd::vector<ActionOverride> PopulateActionsImpl();

            // ToolsApplicationNotificationBus
            void AfterUndoRedo() override;

            AZ::EntityComponentIdPair m_entityComponentIdPair; ///< Entity and Component Id associated with this ComponentMode.
            AZ::Uuid m_componentType; ///< The underlying type of the Component this ComponentMode is for.
        };
    } // namespace ComponentModeFramework
} // namespace AzToolsFramework
