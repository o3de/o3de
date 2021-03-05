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

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string.h>

namespace ScriptCanvas
{
    class Graph;

    namespace Translation
    {
        AZ::Outcome<void, AZStd::string> ToCPlusPlusAndLua(const Graph& graph, const AZStd::string& name, const AZStd::string& path);
        
        AZ::Outcome<void, AZStd::string> ToCPlusPlus(const Graph& graph, const AZStd::string& name, const AZStd::string& path);
        
        AZ::Outcome<void, AZStd::string> ToLua(const Graph& graph, const AZStd::string& name, const AZStd::string& path);
    } // namespace Translation

} // namespace ScriptCanvas