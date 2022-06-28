/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzToolsFramework/ViewportUi/Button.h>
#include <AzToolsFramework/ViewportUi/ViewportUiRequestBus.h>

namespace AZ
{
    class Component;
}

namespace AzToolsFramework
{
    struct SwitcherCluster;

    namespace ViewportUi
    {

        //struct ComponentModeComponent
        //{
        //    ComponentModeComponent() = default;
        //    // disable copying and moving (implicit)
        //    ComponentModeComponent(const ComponentModeComponent&) = delete;


        //    AZ::EntityComponentIdPair entityComponentIdPair
        //    //AZStd::vector<ViewportUi::ButtonId> m_switcherButtonsId; //!< Vector of component mode switcher button ids.
        //};

        class ViewportSwitcherManager
            : private AZ::TickBus::Handler
        {
        public:
            ViewportSwitcherManager(SwitcherCluster* switcherCluster)
                : m_switcherCluster(switcherCluster)
            {
                AZ::TickBus::Handler::BusConnect();
            }

            ~ViewportSwitcherManager()
            {
                AZ::TickBus::Handler::BusDisconnect();
            }
        private:

            // AZ::TickBus::Handler
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
            int GetTickOrder() override;

            void AddSwitcherButton(AZ::EntityComponentIdPair, const char*);
            AZStd::unordered_map<AZ::EntityComponentIdPair, AZ::Component*> GetComponentModeIds(AZ::Entity*);

            //Member variables
            SwitcherCluster* m_switcherCluster = nullptr;
            AZStd::unordered_map<AZ::EntityComponentIdPair, AZStd::string> m_buttons;
        };

    } // namespace AzToolsFramework::ViewportUi
}
