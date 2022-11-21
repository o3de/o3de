/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Core.h>

namespace ScriptEvents
{
    namespace Editor
    {
        void ClearStatusAction(const ScriptCanvas::SourceHandle& sourceHandle);

        bool MakeHelpersAction(const ScriptCanvas::SourceHandle& sourceHandle);

        ScriptCanvas::SourceHandle OpenAction(const AZ::IO::Path& sourceFilePath);

        ScriptCanvas::SourceHandle SaveAction(const ScriptCanvas::SourceHandle& sourceHandle, bool saveInPlace);

        struct MenuItemsEnabled
        {
            bool m_addHelpers = false;
            bool m_clear = false;
            bool m_save = false;
            bool m_saveAs = false;
        };

        MenuItemsEnabled UpdateMenuItemsEnabled(const ScriptCanvas::SourceHandle& sourceHandle);
    }
}
