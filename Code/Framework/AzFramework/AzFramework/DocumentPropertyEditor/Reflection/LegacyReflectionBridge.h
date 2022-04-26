/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/DocumentPropertyEditor/Reflection/Attribute.h>
#include <AzFramework/DocumentPropertyEditor/Reflection/Visitor.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace AZ::Reflection
{
    namespace DescriptorAttributes
    {
        extern Name Handler;
        extern Name Label;
        extern Name SerializedPath;
    }

    template<typename T>
    Dom::Value CreateValue(const T& value)
    {
        return Dom::Value(value);
    }

    template<>
    Dom::Value CreateValue(const AZStd::string_view& value)
    {
        return Dom::Value(value, true);
    }

    template <>
    Dom::Value CreateValue(const AZStd::string& value)
    {
        return Dom::Value(value, true);
    }

    void VisitLegacyInMemoryInstance(IRead* visitor, void* instance, const AZ::TypeId& typeId, AZ::SerializeContext* serializeContext = nullptr);
    void VisitLegacyInMemoryInstance(IReadWrite* visitor, void* instance, const AZ::TypeId& typeId, AZ::SerializeContext* serializeContext = nullptr);
    // void VisitLegacyJsonSerializedInstance(IRead* visitor, Dom::Value instance, const AZ::TypeId& typeId);
}
