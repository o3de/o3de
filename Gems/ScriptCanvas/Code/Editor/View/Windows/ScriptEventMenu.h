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

        AZStd::pair<bool, AZStd::string> MakeHelpersAction(const ScriptCanvas::SourceHandle& sourceHandle);

        AZStd::pair<ScriptCanvas::SourceHandle, AZStd::string> OpenAction();

        AZStd::pair<bool, AZStd::vector<AZStd::string>> ParseAsAction(const ScriptCanvas::SourceHandle& sourceHandle);

        AZStd::pair<bool, AZStd::string> SaveAsAction(const ScriptCanvas::SourceHandle& sourceHandle);

        struct MenuItemsEnabled
        {
            bool m_addHelpers = false;
            bool m_clear = false;
            bool m_parse = false;
            bool m_save = false;
        };

        MenuItemsEnabled UpdateMenuItemsEnabled(const ScriptCanvas::SourceHandle& sourceHandle);
    }
}
