/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// AZ
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>

// GraphModel
#include <GraphModel/Model/DataType.h>

namespace GraphModel
{
    void DataType::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<DataType>()
                ->Version(0)
                ->Field("m_typeEnum", &DataType::m_typeEnum)
                ->Field("m_typeUuid", &DataType::m_typeUuid)
                ->Field("m_defaultValue", &DataType::m_defaultValue)
                ->Field("m_cppName", &DataType::m_cppName)
                ->Field("m_displayName", &DataType::m_displayName)
                ;

            serializeContext->RegisterGenericType<DataTypePtr>();
            serializeContext->RegisterGenericType<DataTypeList>();
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<DataType>("GraphModelDataType")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "editor.graph")
                ->Method("IsValid", &DataType::IsValid)
                ->Method("GetTypeEnum", &DataType::GetTypeEnum)
                ->Method("GetTypeUuid", &DataType::GetTypeUuid)
                ->Method("GetTypeUuidString", &DataType::GetTypeUuidString)
                ->Method("GetDefaultValue", &DataType::GetDefaultValue)
                ->Method("GetDisplayName", &DataType::GetDisplayName)
                ->Method("GetCppName", &DataType::GetCppName)
                ->Method("IsSupportedType", &DataType::IsSupportedType)
                ->Method("IsSupportedValue", &DataType::IsSupportedValue)
               ;
        }
    }

    DataType::DataType(
        Enum typeEnum,
        const AZ::Uuid& typeUuid,
        const AZStd::any& defaultValue,
        AZStd::string_view typeDisplayName,
        AZStd::string_view cppTypeName,
        const AZStd::function<bool(const AZStd::any&)>& valueValidator)
        : m_typeEnum(typeEnum)
        , m_typeUuid(typeUuid)
        , m_defaultValue(defaultValue)
        , m_displayName(typeDisplayName)
        , m_cppName(cppTypeName)
        , m_valueValidator(valueValidator)
    {
    }

    bool DataType::IsValid() const
    {
        return ENUM_INVALID != m_typeEnum && !m_typeUuid.IsNull();
    }

    DataType::Enum DataType::GetTypeEnum() const
    {
        return m_typeEnum;
    }

    const AZ::Uuid& DataType::GetTypeUuid() const
    {
        return m_typeUuid;
    }

    AZStd::string DataType::GetTypeUuidString() const
    {
        return GetTypeUuid().ToString<AZStd::string>();
    }

    bool DataType::operator==(const DataType& other) const
    {
        return m_typeEnum != ENUM_INVALID && other.m_typeEnum != ENUM_INVALID && m_typeEnum == other.m_typeEnum;
    }

    bool DataType::operator!=(const DataType& other) const
    {
        return !(*this == other);
    }

    const AZStd::any& DataType::GetDefaultValue() const
    {
        return m_defaultValue;
    }

    const AZStd::string& DataType::GetDisplayName() const
    {
        return m_displayName;
    }

    const AZStd::string& DataType::GetCppName() const
    {
        return m_cppName;
    }

    bool DataType::IsSupportedType(const AZ::Uuid& typeUuid) const
    {
        return typeUuid == m_typeUuid || typeUuid == m_defaultValue.type();
    }

    bool DataType::IsSupportedValue(const AZStd::any& value) const
    {
        return IsSupportedType(value.type()) && (!m_valueValidator || m_valueValidator(value));
    }
} // namespace GraphModel
