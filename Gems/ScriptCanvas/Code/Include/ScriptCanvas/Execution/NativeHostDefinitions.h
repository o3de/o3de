/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "NativeHostDeclarations.h"

namespace ScriptCanvas
{
    typedef void (*GraphStartFunction)(const RuntimeContext&);

    using GraphStartFunction = void(*)(const RuntimeContext&);

    bool CallNativeGraphStart(AZStd::string_view name, const RuntimeContext& context);
    
    bool RegisterNativeGraphStart(AZStd::string_view name, GraphStartFunction function);
    
    // this may never have to be necessary
    bool UnregisterNativeGraphStart(AZStd::string_view name);

}
