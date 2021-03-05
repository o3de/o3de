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