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

        struct ComponentInfo
        {
            ComponentInfo(AZ::EntityComponentIdPair pairId);
             
            // disable copying and moving (implicit)
            //ComponentInfo(const ComponentInfo&);

            AZ::EntityComponentIdPair m_pairId;
            AZ::Entity* m_entity = nullptr;
            AZ::Component* m_component = nullptr;
            AZ::ComponentDescriptor* m_componentDescriptor = nullptr;
            ViewportUi::ButtonId m_buttonId;
            // AZStd::vector<ViewportUi::ButtonId> m_switcherButtonsId; //!< Vector of component mode switcher button ids.
        };

        class ComponentModeSwitcher
        {
        public:
            ComponentModeSwitcher(Switcher* switcher)
                : m_switcher(switcher)
            {
            }

            ~ComponentModeSwitcher() = default;

            bool Update(EntityIdList selectedEntityIds, EntityIdList deselectedEntityIds);
            void UpdateComponentButton(AZ::EntityComponentIdPair, bool addRemove);

        private:
            void AddSwitcherButton(ComponentInfo*, const char*, const char*);
            void AddComponentButton(AZ::EntityComponentIdPair);
            void RemoveComponentButton(AZ::EntityComponentIdPair);
            // void RemoveSwitcherButton(AZ::EntityComponentIdPair, const char*);

            AZStd::unordered_map<AZ::EntityComponentIdPair, AZ::Component*> GetComponentModeIds(AZ::Entity*);

            // Member variables
            Switcher* m_switcher = nullptr;
            AZStd::unordered_map<AZ::EntityComponentIdPair, AZStd::string> m_buttons;
            AZStd::vector<ComponentInfo> m_addedComponents;
        };

    } // namespace ComponentModeFramework
} // namespace AzToolsFramework
