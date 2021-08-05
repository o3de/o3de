/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
