/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ViewportUi/Button.h>
#include <AzToolsFramework/ViewportUi/ViewportUiRequestBus.h>
#include <AzToolsFramework\Entity\EntityTypes.h>


namespace AZ
{
    class Component;
}
namespace AzToolsFramework
{

    struct Switcher;

    namespace ComponentModeFramework
    {
        // Struct to contain relevant information about component 
        struct ComponentData
        {
            ComponentData() = default;
            ComponentData(AZ::EntityComponentIdPair pairId);

            // Member variables
            AZ::EntityComponentIdPair m_pairId; //!< ID of entity component pair.
            AZ::Entity* m_entity = nullptr; //!< Pointer to entity associated with pairId.
            AZ::Component* m_component = nullptr; //!< Pointer to component associated with pairId.
            AZ::ComponentDescriptor* m_componentDescriptor = nullptr; //!< Pointer to the component descriptor.
            ViewportUi::ButtonId m_buttonId; //!< Button ID of switcher component.
        };

        class ComponentModeSwitcher
        {
        public:
            ComponentModeSwitcher(Switcher* switcher);

            ~ComponentModeSwitcher() = default;
            /// Updates the switcher based on the entity selected
            /// <param name="selectedEntityIds"></param>
            /// <param name="deselectedEntityIds"></param>
            void Update(EntityIdList selectedEntityIds, EntityIdList deselectedEntityIds);
            void UpdateComponentButton(AZ::EntityComponentIdPair, bool addRemove);

            void ActivateComponentMode(ViewportUi::ButtonId);

            AZ::Event<ViewportUi::ButtonId>::Handler m_handler;
        private:

            void AddSwitcherButton(ComponentData&, const char*, const char*);
            void AddComponentButton(AZ::EntityComponentIdPair);
            void RemoveComponentButton(AZ::EntityComponentIdPair);

            AZStd::unordered_map<AZ::EntityComponentIdPair, AZ::Component*> GetComponentModeIds(AZ::Entity*);

            // Member variables
            Switcher* m_switcher = nullptr; //!< Pointer to the UI switcher.
            AZStd::vector<ComponentData> m_addedComponents; //!< Vector of ComponentData elements
            
        };

    } // namespace ComponentModeFramework
} // namespace AzToolsFramework
