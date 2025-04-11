/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Widgets/EditorContextMenu/EditorContextMenu.h>

namespace GraphCanvas
{
    class SlotContextMenu
        : public EditorContextMenu
    {
    public:
        AZ_CLASS_ALLOCATOR(SlotContextMenu, AZ::SystemAllocator);

        SlotContextMenu(EditorId editorId, QWidget* parent = nullptr);
        ~SlotContextMenu() override = default;
    };
}
