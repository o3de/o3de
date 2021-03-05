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

#include "BoxComponentMode.h"

namespace AzToolsFramework
{
    AZ_CLASS_ALLOCATOR_IMPL(BoxComponentMode, AZ::SystemAllocator, 0)

    BoxComponentMode::BoxComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid componentType)
        : EditorBaseComponentMode(entityComponentIdPair, componentType)
    {
        m_boxEdit.Setup(entityComponentIdPair);
    }

    BoxComponentMode::~BoxComponentMode()
    {
        m_boxEdit.Teardown();
    }

    void BoxComponentMode::Refresh()
    {
        m_boxEdit.UpdateManipulators();
    }
} // namespace AzToolsFramework