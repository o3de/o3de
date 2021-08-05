/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ComponentModeCommand.h"

namespace AzToolsFramework
{
    namespace ComponentModeFramework
    {
        static void HandleTransitionToFromComponentMode(
            const AZStd::vector<EntityAndComponentModeBuilders>& componentModeBuilders,
            ComponentModeCommand::Transition transition)
        {
            switch (transition)
            {
            case ComponentModeCommand::Transition::Enter:
                ComponentModeSystemRequestBus::Broadcast(
                    &ComponentModeSystemRequests::BeginComponentMode,
                    componentModeBuilders);
                break;
            case ComponentModeCommand::Transition::Leave:
                ComponentModeSystemRequestBus::Broadcast(
                    &ComponentModeSystemRequests::EndComponentMode);
                break;
            default:
                break;
            }
        }

        // mirror/swap the incoming transition
        // enter -> leave, leave -> enter
        static ComponentModeCommand::Transition Mirror(
            const ComponentModeCommand::Transition transition)
        {
            switch (transition)
            {
            case ComponentModeCommand::Transition::Enter:
                return ComponentModeCommand::Transition::Leave;
            case ComponentModeCommand::Transition::Leave:
                return ComponentModeCommand::Transition::Enter;
            default:
                AZ_Error("ComponentMode", false, "Invalid ComponentMode transition passed");
                return ComponentModeCommand::Transition::Enter;
            }
        }

        ComponentModeCommand::ComponentModeCommand(
            const Transition transition, const AZStd::string& friendlyName,
            const AZStd::vector<EntityAndComponentModeBuilders>& componentModeBuilders)
                : URSequencePoint(friendlyName)
                , m_componentModeBuilders(componentModeBuilders)
                , m_transition(transition) {}

        void ComponentModeCommand::Undo()
        {
            HandleTransitionToFromComponentMode(
                m_componentModeBuilders, Mirror(m_transition));
        }

        void ComponentModeCommand::Redo()
        {
            HandleTransitionToFromComponentMode(
                m_componentModeBuilders, m_transition);
        }
    } // namespace ComponentModeFramework
}  // namespace AzToolsFramework
