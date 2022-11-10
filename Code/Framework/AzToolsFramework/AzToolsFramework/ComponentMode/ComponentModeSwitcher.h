/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Viewport/ViewportBus.h>
#include <AzToolsFramework/API/EntityCompositionNotificationBus.h>
#include <AzToolsFramework/API/ViewportEditorModeTrackerNotificationBus.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <AzToolsFramework/Entity/EntityTypes.h>
#include <AzToolsFramework/ViewportUi/Button.h>
#include <AzToolsFramework/ComponentMode/ComponentModeDelegateBus.h>

namespace AZ
{
    class Component;
}

namespace AzToolsFramework
{
    namespace ComponentModeFramework
    {
        //! Struct containing relevant information about component for the switcher.
        struct ComponentData
        {
            ComponentData() = default;
            ComponentData(AZ::EntityComponentIdPair pairId);

            // Member variables
            AZ::EntityComponentIdPair m_pairId; //!< Id of entity component pair.
            AZ::Entity* m_entity = nullptr; //!< Pointer to entity associated with pairId.
            AZ::Component* m_component = nullptr; //!< Pointer to component associated with pairId.
            AZStd::string m_componentName; //!< Friendly name of component.
            AZStd::string m_iconPath; //!< Path of component icon.
            ViewportUi::ButtonId m_buttonId; //!< Button Id of switcher component.
        };

        //! Handles all aspects of the ViewportUi Switcher related to Component Mode.
        class ComponentModeSwitcher
            : private EditorComponentModeNotificationBus::Handler
            , private ViewportEditorModeNotificationsBus::Handler
            , private EntityCompositionNotificationBus::Handler
            , private ToolsApplicationNotificationBus::Handler
            , private AzFramework::ViewportImGuiNotificationBus::Handler
            , private AzFramework::ComponentModeDelegateNotificationBus::Handler
        {
        public:
            ComponentModeSwitcher();
            ~ComponentModeSwitcher();

            size_t GetComponentCount() const
            {
                return m_addedComponents.size();
            }

            // Returns a null pointer if not in component mode
            const AZ::Component* GetActiveComponent() const
            {
                return m_activeSwitcherComponent;
            }

            AZ::Component* GetActiveComponent()
            {
                return m_activeSwitcherComponent;
            }

            ViewportUi::SwitcherId GetSwitcherId() const
            {
                return m_switcherId;
            }

        private:
            //! Calls ViewportUiRequestBus to create switcher button, helper for AddComponentButton.
            void AddSwitcherButton(ComponentData& componentData);
            //! Adds component button to switcher.
            void AddComponentButton(const AZ::EntityComponentIdPair pairId);
            //! Removes component button from switcher.
            void RemoveComponentButton(const AZ::EntityComponentIdPair pairId);
            //! Add or remove component buttons to/from the switcher based on entities selected.
            void UpdateSwitcher(
                const EntityIdList& newlyselectedEntityIds, const EntityIdList& newlydeselectedEntityIds);
            //! Clears all buttons from switcher.
            void ClearSwitcher();
            //! Remove all components from the switcher that don't exist on all entities.
            void RemoveNonCommonComponents(const AZ::Entity& entity);
            void ActivateComponentMode(const ViewportUi::ButtonId buttonId);

            // ViewportEditorModeNotificationsBus overrides ...
            void OnEditorModeActivated(
                [[maybe_unused]] const ViewportEditorModesInterface& editorModeState, ViewportEditorMode mode) override;
            void OnEditorModeDeactivated(
                [[maybe_unused]] const ViewportEditorModesInterface& editorModeState, ViewportEditorMode mode) override;

            // ToolsApplicationBus overrides ...
            void AfterEntitySelectionChanged(
                const EntityIdList& newlySelectedEntities, const EntityIdList& newlyDeselectedEntities) override;

            // ViewportImGuiNotificationBus overrides ...
            void OnImGuiDropDownShown() override;
            void OnImGuiDropDownHidden() override;
            void OnImGuiActivated() override;
            void OnImGuiDeactivated() override;

            // ComponentModeDelegateNotificationBus overrides ...
            void OnComponentModeDelegateConnect(const AZ::EntityComponentIdPair& pairId) override;
            void OnComponentModeDelegateDisconnect(const AZ::EntityComponentIdPair& pairId) override;

            // Member variables
            AZ::Component* m_activeSwitcherComponent = nullptr; //!< The component that is currently in component mode
            AZStd::vector<ComponentData> m_addedComponents; //!< Vector of ComponentData elements..
            ViewportUi::ButtonId m_transformButtonId; //!< Id of the default button of the switcher, used to exit component mode.
            AZ::Event<ViewportUi::ButtonId>::Handler m_handler; //!< Handler for onclick of switcher buttons, activates component mode.
            ViewportUi::SwitcherId m_switcherId; //!< Id of linked switcher.
            AZ::EntityComponentIdPair m_componentModePair; //!< The component mode pair in onEntityCompositionChanged.
            //! Protects the switcher from being opened by OnImGuiDropDownShown if it has been hidden elsewhere.
            bool m_hiddenByImGui = false;
        };

    } // namespace ComponentModeFramework
} // namespace AzToolsFramework
