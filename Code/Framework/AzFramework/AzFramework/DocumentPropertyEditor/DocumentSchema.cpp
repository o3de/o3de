/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzFramework/DocumentPropertyEditor/DocumentSchema.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorSystemInterface.h>

namespace AZ::DocumentPropertyEditor
{
    Dom::Value TypeIdAttributeDefinition::ValueToDom(const AZ::TypeId& attribute) const
    {
        return AZ::Dom::Utils::TypeIdToDomValue(attribute);
    }

    AZStd::optional<AZ::TypeId> TypeIdAttributeDefinition::DomToValue(const Dom::Value& value) const
    {
        // If we happen to see a type ID marshalled as an AZStd::any, go ahead and bring it in.
        if (value.IsOpaqueValue())
        {
            return AttributeDefinition<AZ::TypeId>::DomToValue(value);
        }
        if (value.IsString())
        {
            auto typeId = AZ::Dom::Utils::DomValueToTypeId(value);
            if (typeId.IsNull())
            {
                return {};
            }
            return typeId;
        }
        return {};
    }

    AZStd::shared_ptr<AZ::Attribute> TypeIdAttributeDefinition::DomValueToLegacyAttribute(const AZ::Dom::Value& value, bool) const
    {
        AZ::Uuid uuidValue = DomToValue(value).value_or(AZ::Uuid::CreateNull());
        return AZStd::make_shared<AZ::AttributeData<AZ::Uuid>>(AZStd::move(uuidValue));
    }

    AZ::Dom::Value TypeIdAttributeDefinition::LegacyAttributeToDomValue(AZ::PointerObject instanceObject, AZ::Attribute* attribute) const
    {
        AZ::AttributeReader reader(instanceObject.m_address, attribute);
        AZ::Uuid value;
        if (!reader.Read<AZ::Uuid>(value))
        {
            return AZ::Dom::Value();
        }
        return AZ::Dom::Utils::TypeIdToDomValue(value);
    }

    Dom::Value NamedCrcAttributeDefinition::ValueToDom(const AZ::Name& attribute) const
    {
        return Dom::Value(attribute.GetStringView(), true);
    }

    AZStd::optional<AZ::Name> NamedCrcAttributeDefinition::DomToValue(const Dom::Value& value) const
    {
        if (value.IsString())
        {
            return AZ::Name(value.GetString());
        }
        return {};
    }

    AZStd::shared_ptr<AZ::Attribute> NamedCrcAttributeDefinition::DomValueToLegacyAttribute(const AZ::Dom::Value& value, bool) const
    {
        AZ::Crc32 crc = 0;
        if (value.IsString())
        {
            AZStd::string_view valueString = value.GetString();
            crc = AZ::Crc32(valueString.data(), valueString.size());
        }
        return AZStd::make_shared<AZ::AttributeData<AZ::Crc32>>(crc);
    }

    AZ::Dom::Value NamedCrcAttributeDefinition::LegacyAttributeToDomValue(AZ::PointerObject instanceObject, AZ::Attribute* attribute) const
    {
        AZ::Name result;
        AZ::AttributeReader reader(instanceObject.m_address, attribute);

        AZ::Crc32 valueCrc = 0;
        if (!reader.Read<AZ::Crc32>(valueCrc))
        {
            AZ::u32 valueU32 = 0;
            reader.Read<AZ::u32>(valueU32);
            valueCrc = valueU32;
        }

        if (static_cast<AZ::u32>(valueCrc) != 0)
        {
            auto propertyEditorSystem = AZ::Interface<PropertyEditorSystemInterface>::Get();
            AZ_Assert(propertyEditorSystem != nullptr, "Attempted to retrieve a CRC32 value without PropertyEditorSystem");
            if (propertyEditorSystem != nullptr)
            {
                result = propertyEditorSystem->LookupNameFromId(valueCrc);
            }
        }

        return Dom::Value(result.GetStringView(), false);
    }
} // namespace AZ::DocumentPropertyEditor
