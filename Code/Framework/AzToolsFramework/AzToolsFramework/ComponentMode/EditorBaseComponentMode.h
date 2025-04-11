/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <AzToolsFramework/Prefab/PrefabPublicNotificationBus.h>
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
            , public Prefab::PrefabPublicNotificationBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR_DECL
            AZ_RTTI(EditorBaseComponentMode, "{7E979280-42A5-4D93-BB23-BBA8A5E48146}")

            /// @cond
            EditorBaseComponentMode(
                const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType);
            EditorBaseComponentMode(EditorBaseComponentMode&&) = default;
            EditorBaseComponentMode& operator=(EditorBaseComponentMode&&) = default;
            virtual ~EditorBaseComponentMode();
            /// @endcond

            static void Reflect(AZ::ReflectContext* context);

            /// ComponentMode interface - populate actions for this ComponentMode.
            /// When PopulateActions is called, if a second action override is found with the
            /// same key, it should override the existing action if one already exists.
            /// (e.g. The 'escape' key will first deselect a vertex, then leave ComponentMode if
            /// an action is added to deselect a vertex when one is selected)
            /// @attention More specific actions come later in the ordering when they are added.
            AZStd::vector<ActionOverride> PopulateActions() final;

            //! Register additional actions for this component mode.
            //! This is provided as guidance but it will not be called automatically;
            //! you will need a system component to call it in the OnActionRegistrationHook handler.
            static void RegisterActions() {}

            //! Bind actions to appear in this component mode.
            //! This is provided as guidance but it will not be called automatically;
            //! you will need a system component to call it in the OnActionContextModeBindingHook handler.
            static void BindActionsToModes() {}

            //! Bind actions to appear in menus.
            //! This is provided as guidance but it will not be called automatically;
            //! you will need a system component to call it in the OnMenuBindingHook handler.
            static void BindActionsToMenus() {}

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

            // Default implementation of EditorComponentModeBus handler to aid developers.
            AZ::Uuid GetComponentModeType() const;

            /// EditorBaseComponentMode interface
            /// @see To be overridden by derived ComponentModes
            virtual AZStd::vector<ViewportUi::ClusterId> PopulateViewportUiImpl();

        private:
            /// EditorBaseComponentMode interface
            /// @see To be overridden by derived ComponentModes
            virtual AZStd::vector<ActionOverride> PopulateActionsImpl();

            // PrefabPublicNotificationBus overrides ...
            void OnPrefabInstancePropagationEnd() override;

            AZ::EntityComponentIdPair m_entityComponentIdPair; ///< Entity and Component Id associated with this ComponentMode.
            AZ::Uuid m_componentType; ///< The underlying type of the Component this ComponentMode is for.
        };

        //! Templated helper to implement the Reflect function on classes derived from EditorBaseComponentMode
        template<typename EditorBaseComponentModeDescendantClass>
        void ReflectEditorBaseComponentModeDescendant(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorBaseComponentModeDescendantClass, EditorBaseComponentMode>(
                    AZ::Internal::NullFactory::GetInstance());
            }
        }
    } // namespace ComponentModeFramework
} // namespace AzToolsFramework
