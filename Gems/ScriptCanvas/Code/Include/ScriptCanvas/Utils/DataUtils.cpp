/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Serialization/Utils.h>

#include <ScriptCanvas/Utils/DataUtils.h>

namespace ScriptCanvas
{
    //////////////
    // DataUtils
    //////////////
    
    bool DataUtils::MatchesDynamicDataType(const DynamicDataType& dynamicDataType, const Data::Type& dataType)
    {
        if (dynamicDataType == DynamicDataType::Any)
        {
            return true;
        }
        
        bool isContainerType = AZ::Utils::IsContainerType(ScriptCanvas::Data::ToAZType(dataType));
        
        if (dynamicDataType == DynamicDataType::Container && !isContainerType)
        {
            return false;
        }
        else if (dynamicDataType == DynamicDataType::Value && isContainerType)
        {
            return false;
        }
        
        return true;
    }
    
    AZ::Outcome<void, AZStd::string> DataUtils::MatchesDynamicDataTypeOutcome(const DynamicDataType& dynamicDataType, const Data::Type& dataType)
    {
        if (MatchesDynamicDataType(dynamicDataType, dataType))
        {
            return AZ::Success();
        }        
        
        if (dynamicDataType == DynamicDataType::Container)
        {
            return AZ::Failure(AZStd::string::format("%s is not a Container type.", ScriptCanvas::Data::GetName(dataType).c_str()));
        }
        else if (dynamicDataType == DynamicDataType::Value)
        {
            return AZ::Failure(AZStd::string::format("%s is a Container type and cannot be pushed as a value.", ScriptCanvas::Data::GetName(dataType).c_str()));
        }

        return AZ::Failure(AZStd::string("Unknown failure condition found"));
    }
}
