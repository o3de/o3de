/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <AzToolsFramework/Entity/EntityTypes.h>
#include <AzToolsFramework/ViewportUi/Button.h>
#include <AzToolsFramework/ViewportUi/ViewportUiRequestBus.h>
#include <AzToolsFramework/API/ViewportEditorModeTrackerNotificationBus.h>
#include <AzToolsFramework/API/EntityCompositionNotificationBus.h>

namespace AZ
{
    class Component;
}
namespace AzToolsFramework
{
    namespace ComponentModeFramework
    {
        // Struct containing relevant information about component for the switcher
        struct ComponentData
        {
            ComponentData() = default;
            ComponentData(AZ::EntityComponentIdPair pairId);

            // Member variables
            AZ::EntityComponentIdPair m_pairId; //!< ID of entity component pair.
            AZ::Entity* m_entity = nullptr; //!< Pointer to entity associated with pairId.
            AZ::Component* m_component = nullptr; //!< Pointer to component associated with pairId.
            AZStd::string m_componentName; //!< Freindly name of component.
            AZStd::string m_iconStr; //!< Path of icon.
            ViewportUi::ButtonId m_buttonId; //!< Button ID of switcher component.
        };

        struct Switcher
        {
            Switcher() = default;
            // disable copying and moving (implicit)
            Switcher(const Switcher&) = delete;
            Switcher& operator=(const Switcher&) = delete;

            ViewportUi::SwitcherId m_switcherId; //!< Switcher id.
            AZ::Event<ViewportUi::ButtonId>::Handler m_switcherHandler; //!< Callback for when a switcher button is pressed.
        };

        class ComponentModeSwitcher
            : private EditorComponentModeNotificationBus::Handler
            , private ViewportEditorModeNotificationsBus::Handler
            , private EntityCompositionNotificationBus::Handler
            , private ToolsApplicationNotificationBus::Handler
        {
        public:
            ComponentModeSwitcher();
            ~ComponentModeSwitcher();

            // Add or remove component buttons to/from the switcher based on entities selected.
            void Update(EntityIdList selectedEntityIds, EntityIdList deselectedEntityIds);
            void AddComponentButton(AZ::EntityComponentIdPair);
            // Removes component button
            void RemoveComponentButton(AZ::EntityComponentIdPair);
            std::size_t GetComponentCount() const { return m_addedComponents.size();};
            
            // Handler for the entering component mode.
        protected:
            Switcher m_switcher; //!< Related viewport ui state for controlling the component mode switcher.

        private:
            // Calls ViewportUiRequestBus to create switcher button, helper for AddComponentButton.
            void AddSwitcherButton(ComponentData&, const char* componentName, const char* iconStr);
            // Creates button with component information.
            //void AddComponentButton(AZ::EntityComponentIdPair);
            //// Removes component button
            //void RemoveComponentButton(AZ::EntityComponentIdPair);
            //// Uses ComponentModeSystemRequestBus to initiate component mode.
            void ActivateComponentMode(ViewportUi::ButtonId);
            // Removes swticher buttons that are not common to the inputted entity.
            void RemoveExclusiveComponents(const AZ::Entity&);

            // EditorComponentModeNotificationBus overrides ...
            void OnComponentModeExit() override;

            // ViewportEditorModeNotificationsBus overrides ...
            void OnEditorModeActivated(
                [[maybe_unused]] const ViewportEditorModesInterface& editorModeState, [[maybe_unused]] ViewportEditorMode mode) override;
            void OnEditorModeDeactivated(
                [[maybe_unused]] const ViewportEditorModesInterface& editorModeState, [[maybe_unused]] ViewportEditorMode mode) override;

            // EntityCompositionNotificationBus overrides ...
            void OnEntityComponentAdded(const AZ::EntityId& /*entityId*/, const AZ::ComponentId& /*componentId*/) override;
            void OnEntityComponentRemoved(const AZ::EntityId& /*entityId*/, const AZ::ComponentId& /*componentId*/) override;
            void OnEntityComponentEnabled(const AZ::EntityId& /*entityId*/, const AZ::ComponentId& /*componentId*/) override;
            void OnEntityComponentDisabled(const AZ::EntityId& /*entityId*/, const AZ::ComponentId& /*componentId*/) override;
            void OnEntityCompositionChanged(const AzToolsFramework::EntityIdList& /*entityIdList*/) override;

            // ToolsApplicationBus overrides ...
            void AfterEntitySelectionChanged(
                const EntityIdList& newlySelectedEntities, const EntityIdList& newlyDeselectedEntities) override;

            // Member variables
            //Switcher* m_switcher = nullptr; //!< Pointer to the UI switcher.
            AZStd::vector<ComponentData> m_addedComponents; //!< Vector of ComponentData elements
            EntityIdList m_entityIds; //!< List of entities active in the switcher
            AZStd::set<AZStd::string> m_addedComponentNames;
            AZ::EntityComponentIdPair m_componentModePair;
            bool m_AddRemove;
            ViewportUi::ButtonId m_transformButtonId;
            AZ::Event<ViewportUi::ButtonId>::Handler m_handler;
        };

    } // namespace ComponentModeFramework
} // namespace AzToolsFramework
