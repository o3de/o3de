/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/API/EntityCompositionNotificationBus.h>
#include <AzToolsFramework/API/ViewportEditorModeTrackerNotificationBus.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <AzToolsFramework/Entity/EntityTypes.h>
#include <AzToolsFramework/ViewportUi/Button.h>
#include <AzToolsFramework/ViewportUi/ViewportUiRequestBus.h>

namespace AZ
{
    class Component;
}
namespace AzToolsFramework
{
    namespace ComponentModeFramework
    {

        enum class AddOrRemoveComponent : AZ::u8
        {
            Add,
            Remove
        };

        // Struct containing relevant information about component for the switcher
        struct ComponentData
        {
            ComponentData() = default;
            ComponentData(AZ::EntityComponentIdPair pairId);

            // Member variables
            AZ::EntityComponentIdPair m_pairId; //!< Id of entity component pair.
            AZ::Entity* m_entity = nullptr; //!< Pointer to entity associated with pairId.
            AZ::Component* m_component = nullptr; //!< Pointer to component associated with pairId.
            AZStd::string m_componentName; //!< Freindly name of component.
            AZStd::string m_iconStr; //!< Path of component icon.
            ViewportUi::ButtonId m_buttonId; //!< Button Id of switcher component.
        };

        struct Switcher
        {
            Switcher() = default;
            // disable copying and moving (implicit)
            Switcher(const Switcher&) = delete;
            Switcher& operator=(const Switcher&) = delete;

            ViewportUi::SwitcherId m_switcherId; //!< Id of linked switcher.
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

            std::size_t GetComponentCount() const
            {
                return m_addedComponents.size();
            };

            ComponentData* GetActiveComponent() const
            {
                return m_activeSwitcherComponent;
            };

            // Handler for the entering component mode.
            

        private:
            // Calls ViewportUiRequestBus to create switcher button, helper for AddComponentButton.
            void AddSwitcherButton(ComponentData&, const char* componentName, const char* iconStr);
            // Adds component button to switcher.
            void AddComponentButton(AZ::EntityComponentIdPair);
            // Removes component button from switcher.
            void RemoveComponentButton(AZ::EntityComponentIdPair);
            // Add or remove component buttons to/from the switcher based on entities selected.
            void UpdateSwitcherOnEntitySelectionChange(EntityIdList newlyselectedEntityIds, EntityIdList newlydeselectedEntityIds);
            // Uses ComponentModeSystemRequestBus to initiate component mode.
            void ActivateComponentMode(ViewportUi::ButtonId);
            // Removes swticher buttons that are not common to all selected entities.
            void RemoveExclusiveComponents(const AZ::Entity&);

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
            ComponentData* m_activeSwitcherComponent = nullptr; //!< The component that is currently in component mode

            AZStd::vector<ComponentData> m_addedComponents; //!< Vector of ComponentData elements.
            EntityIdList m_entityIds; //!< List of entities active in the switcher.
            ViewportUi::ButtonId m_transformButtonId; //!< Id of the default button of the switcher, used to exit component mode.
            AZ::Event<ViewportUi::ButtonId>::Handler m_handler; //!< Handler for onclick of switcher buttons, activates component mode.
            ViewportUi::SwitcherId m_switcherId; //!< Id of linked switcher.

            AZ::EntityComponentIdPair m_componentModePair; //!< The component mode pair in onEntityCompositionChanged.
            AddOrRemoveComponent m_addOrRemove; //!< Setting to either add or remove component.
        };

    } // namespace ComponentModeFramework
} // namespace AzToolsFramework
