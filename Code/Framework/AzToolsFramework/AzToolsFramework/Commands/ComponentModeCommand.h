/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
            AZ_CLASS_ALLOCATOR(ComponentModeCommand, AZ::SystemAllocator, 0);
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