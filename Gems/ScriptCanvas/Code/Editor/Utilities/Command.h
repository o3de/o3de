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