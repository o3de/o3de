/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/PointerObject.h>
#include <AzFramework/DocumentPropertyEditor/Reflection/Attribute.h>
#include <AzFramework/DocumentPropertyEditor/Reflection/Visitor.h>
#include <AzFramework/DocumentPropertyEditor/Reflection/LegacyReflectionBridgeConstants.h>
#include <AzFramework/AzFrameworkAPI.h>
namespace AZ
{
    class SerializeContext;
}

namespace AZ::Reflection
{
    AZF_API AZStd::shared_ptr<AZ::Attribute> WriteDomValueToGenericAttribute(const AZ::Dom::Value& value);
    AZF_API AZStd::optional<AZ::Dom::Value> ReadGenericAttributeToDomValue(AZ::PointerObject instance, AZ::Attribute* attribute);

    AZF_API void VisitLegacyInMemoryInstance(
        IRead* visitor, void* instance, const AZ::TypeId& typeId, AZ::SerializeContext* serializeContext = nullptr);
        AZF_API void VisitLegacyInMemoryInstance(
        IReadWrite* visitor, void* instance, const AZ::TypeId& typeId, AZ::SerializeContext* serializeContext = nullptr);
    // TODO: Currently we only support reflecting in-memory instances, but we'll also need to be able to reflect JSON-serialized
    // instances to interface with the Prefab System.
    // void VisitLegacyJsonSerializedInstance(IRead* visitor, Dom::Value instance, const AZ::TypeId& typeId);
} // namespace AZ::Reflection

namespace AZ::Reflection::LegacyReflectionInternal
{
    struct AZF_API AttributeData
    {
        AZ_TYPE_INFO(AttributeData, "{EFD2A3A3-8161-4B9C-90B8-952AA08FD961}");

        // Group from the attribute metadata (generally empty with Serialize/EditContext data)
        Name m_group;
        // Name of the attribute
        Name m_name;
        // DOM value of the attribute - currently only primitive attributes are supported,
        // but other types may later be supported via opaque values
        Dom::Value m_value;
    };

    //! Stores information about an associative container element key
    //! This is used by the LegacyReflectionBridge StackEntry
    //! to have the mapped_type of an associative container element to be able
    //! access the instance and attribute data for the corresponding key
    struct AZF_API KeyEntry
    {
        AZ_TYPE_INFO(KeyEntry, "{718537E1-DFF5-4662-AB86-1D5C0C8A0768}");

        //! Stores the address and type ID of an associative container key
        AZ::PointerObject m_keyInstance;
        //! Stores the attributes of a single associative container element key
        AZStd::vector<AttributeData> m_keyAttributes;

        bool IsValid() const;
    };
}
