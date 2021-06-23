/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
