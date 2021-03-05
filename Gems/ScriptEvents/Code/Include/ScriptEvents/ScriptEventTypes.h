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

#include <ScriptEvents/Internal/VersionedProperty.h>

namespace ScriptEvents
{
    namespace Types
    {
        using SupportedTypes = AZStd::vector< AZStd::pair< AZ::Uuid, AZStd::string> >;
        using VersionedTypes = AZStd::vector<AZStd::pair< ScriptEventData::VersionedProperty, AZStd::string>>;

        VersionedTypes GetValidAddressTypes();

        // Returns the list of the valid Script Event method parameter type Ids, this is used 
        AZStd::vector<AZ::Uuid> GetSupportedParameterTypes();

        // Returns the list of the valid Script Event method parameters, this is used to populate the ReflectedPropertyEditor's combobox
        VersionedTypes GetValidParameterTypes();

        // Determines whether the specified type is a valid parameter on a Script Event method argument list
        bool IsValidParameterType(const AZ::Uuid& typeId);

        // Supported return types for Script Event methods
        AZStd::vector<AZ::Uuid> GetSupportedReturnTypes();

        VersionedTypes GetValidReturnTypes();

        bool IsValidReturnType(const AZ::Uuid& typeId);

        AZ::BehaviorMethod* FindBehaviorOperatorMethod(const AZ::BehaviorClass* behaviorClass, AZ::Script::Attributes::OperatorType operatorType);

        bool IsAddressableType(const AZ::Uuid& uuid);

        bool ValidateAddressType(const AZ::Uuid& addressTypeId);
    }
}
