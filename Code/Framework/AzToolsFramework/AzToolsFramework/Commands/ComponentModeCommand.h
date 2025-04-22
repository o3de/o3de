/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <AzToolsFramework/Undo/UndoSystem.h>

namespace AzToolsFramework
{
    namespace ComponentModeFramework
    {
        /// Undo command to track entering and leaving ComponentMode.
        class ComponentModeCommand
            : public UndoSystem::URSequencePoint
        {
        public:
            AZ_CLASS_ALLOCATOR(ComponentModeCommand, AZ::SystemAllocator);
            AZ_RTTI(ComponentModeCommand, "{6D574E94-4D67-47D8-8C93-825BF82E2A28}");

            /// Type of transition - did we enter or leave ComponentMode.
            enum class Transition
            {
                Enter,
                Leave
            };

            ComponentModeCommand(
                Transition transition, const AZStd::string& friendlyName,
                const AZStd::vector<EntityAndComponentModeBuilders>& componentModeBuilders);

            void Undo() override;
            void Redo() override;

            bool Changed() const override { return true; } // State will always have changed.

        private:
            AZStd::vector<EntityAndComponentModeBuilders> m_componentModeBuilders; /**< List of builders for specific
                                                                                     *  ComponentModes - to be created
                                                                                     *  when entering ComponentMode. */
            Transition m_transition; ///< Entering/Leaving ComponentMode.
        };
    } // namespace ComponentModeFramework
} // namespace AzToolsFramework
