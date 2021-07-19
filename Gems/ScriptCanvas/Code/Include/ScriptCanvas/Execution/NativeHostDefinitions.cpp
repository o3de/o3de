/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "NativeHostDefinitions.h"
#include <AzCore/std/containers/unordered_map.h>

namespace NativeHostDefinitionsCPP
{
    using namespace ScriptCanvas;

    using FunctionMap = AZStd::unordered_map<AZStd::string, GraphStartFunction>;
    FunctionMap s_functionMap;
}

namespace ScriptCanvas
{
    bool CallNativeGraphStart(AZStd::string_view name, const RuntimeContext& context)
    {
        using namespace NativeHostDefinitionsCPP;

        auto iter = s_functionMap.find(name);
        if (iter != s_functionMap.end())
        {
            iter->second(context);
            return true;
        }

        return false;
    }

    bool RegisterNativeGraphStart(AZStd::string_view name, GraphStartFunction function)
    {
        using namespace NativeHostDefinitionsCPP;
        
        auto iter = s_functionMap.find(name);
        if (iter == s_functionMap.end())
        {
            s_functionMap.insert({ name, function });
            return true;
        }
        
        return false;
    }

    // this may never have to be necessary
    bool UnregisterNativeGraphStart(AZStd::string_view name)
    {
        using namespace NativeHostDefinitionsCPP;
        
        auto iter = s_functionMap.find(name);
        if (iter != s_functionMap.end())
        {
            s_functionMap.erase(iter);
            return true;
        }

        return false;
    }

}
