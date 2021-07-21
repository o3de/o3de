/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/std/string/string.h>

namespace ScriptCanvasEditor
{
    // Commands are named wrappers around ebus events
    class Command
    {
    public:

        //template <typename ...args>
        //void Execute(const char* commandName, args&&... parameters);

    private:

        AZStd::string m_commandName;
        AZStd::string m_description;
        AZStd::string m_category;
        AZStd::string m_iconPath;
    };
}
