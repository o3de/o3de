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

#include <AzToolsFramework/ComponentMode/EditorBaseComponentMode.h>
#include <AzToolsFramework/ComponentModes/BoxViewportEdit.h>

namespace AzToolsFramework
{
    class LinearManipulator;

    /// The specific ComponentMode responsible for handling box editing.
    class BoxComponentMode
        : public ComponentModeFramework::EditorBaseComponentMode
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        BoxComponentMode(
            const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType);
        BoxComponentMode(const BoxComponentMode&) = delete;
        BoxComponentMode& operator=(const BoxComponentMode&) = delete;
        BoxComponentMode(BoxComponentMode&&) = delete;
        BoxComponentMode& operator=(BoxComponentMode&&) = delete;
        ~BoxComponentMode();

        // EditorBaseComponentMode
        void Refresh() override;

    private:
        BoxViewportEdit m_boxEdit;
    };
} // namespace AzToolsFramework