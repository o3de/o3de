/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <ScriptCanvas/Core/Core.h>

#include <ScriptCanvas/Core/SlotConfigurations.h>

namespace ScriptCanvas
{
    class DataUtils
    {
    public:
        
        static bool MatchesDynamicDataType(const DynamicDataType& dynamicDataType, const Data::Type& dataType);
        static AZ::Outcome<void, AZStd::string> MatchesDynamicDataTypeOutcome(const DynamicDataType& dynamicDataType, const Data::Type& dataType);
    };
}
