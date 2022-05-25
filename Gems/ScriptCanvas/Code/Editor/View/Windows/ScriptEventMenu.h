/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)

#endif

namespace ScriptEventEditor
{
    struct MenuItemsEnabled
    {
        bool m_save = false;
        bool m_addHelpers = false;
    };
    MenuItemsEnabled UpdateMenuItemsEnabled();
}
