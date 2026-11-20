/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <SentryCrashCore/SentryCrashCore.h>

#include <map>
#include <string>
#include <vector>

namespace CrashHandler
{
    using CrashHandlerAnnotations = std::map<std::string, std::string>;
    using CrashHandlerArguments = std::vector<std::string>;

    //! Same public surface as the Crashpad-backed ToolsCrashHandler
    //! (Code/Tools/CrashHandler/Tools), so the eleven existing callers - the Editor, ProjectManager,
    //! LuaIDE, AssetProcessor, the Atom tools, HammerBox, JoltPVD, ScriptCanvas - keep working
    //! unchanged whichever backend PAL_TRAIT_CRASH_HANDLER_BACKEND selects.
    class ToolsCrashHandler
    {
    public:
        static void InitCrashHandler(
            const std::string& moduleTag,
            const std::string& devRoot,
            const std::string& crashUrl = {},
            const std::string& crashToken = {},
            const std::string& handlerFolder = {},
            const CrashHandlerAnnotations& baseAnnotations = {},
            const CrashHandlerArguments& arguments = {});
    };
}
