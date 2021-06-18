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

#include "ScriptUserDataSerializer.h"

#include <AzCore/Serialization/Json/JsonSerialization.h>

namespace AZ
{
    AZ_CLASS_ALLOCATOR_IMPL(ScriptUserDataSerializer, SystemAllocator, 0);

    //ScriptUserDataSerializer::TypeOptions ScriptUserDataSerializer::GetVariantTypeOptions(AZ::SerializeContext::IDataContainer& container)
    //{
    //    TypeOptions ret;
    //    container.EnumTypes([&ret](const AZ::Uuid& typ, const AZ::SerializeContext::ClassElement* ce)
    //    {
    //        ret.emplace_back(typ, ce);
    //        return true;
    //    });
    //    return ret;
    //}

    //AZStd::optional<ScriptUserDataSerializer::TypeOption> ScriptUserDataSerializer::GetVariantTypeOptionAtIndex(AZ::SerializeContext::IDataContainer& container, size_t index)
    //{
    //    AZStd::optional<TypeOption> ret;
    //    size_t curIndex = 0;

    //    container.EnumTypes([&](const AZ::Uuid& typ, const AZ::SerializeContext::ClassElement* ce)
    //    {
    //        const bool found = curIndex == index;
    //        if (found)
    //        {
    //            ret.emplace(typ, ce);
    //        }
    //        curIndex++;
    //        return !found;
    //    });
    //    return ret;
    //}

    const char* ScriptUserDataSerializer::IndexMemberName()
    {
        return "type";
    };

    const char* ScriptUserDataSerializer::ValueMemberName()
    {
        return "value";
    };

    const char* ScriptUserDataSerializer::PrettyTypeName()
    {
        return "any";
    }

    AZStd::optional<ScriptUserDataSerializer::TypeOption> ScriptUserDataSerializer::GetIndexTypeFromIndex(IDataContainer&, const AZStd::optional<IndexMemberType>& mbIndex)
    {
        if (mbIndex)
        {
            return { TypeOption(*mbIndex, nullptr) };
        }
        else
        {
            return {};
        }
    };

    // Given the type of the stored second component, retrieves the value of the correspond index
    AZStd::optional<ScriptUserDataSerializer::IndexMemberType> ScriptUserDataSerializer::GetIndexFromIndexType(IDataContainer&, const TypeOption& type)
    {
        return { type.first };
    };
}
