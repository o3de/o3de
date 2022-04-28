/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// AZ
#include <AzCore/Serialization/SerializeContext.h>

// GraphModel
#include <GraphModel/Model/DataType.h>

namespace GraphModel
{
    void DataType::Reflect(AZ::ReflectContext* reflection)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
        {
            serializeContext->Class<DataType>()
                ->Version(0)
                ->Field("m_typeEnum", &DataType::m_typeEnum)
                ->Field("m_typeUuid", &DataType::m_typeUuid)
                ->Field("m_defaultValue", &DataType::m_defaultValue)
                ->Field("m_cppName", &DataType::m_cppName)
                ->Field("m_displayName", &DataType::m_displayName)
                ;
        }
    }

    DataType::DataType()
        : m_typeEnum(ENUM_INVALID)
        , m_displayName("INVALID")
        , m_cppName("INVALID")
    {
    }

    DataType::DataType(
        Enum typeEnum,
        const AZ::Uuid& typeUuid,
        const AZStd::any& defaultValue,
        AZStd::string_view typeDisplayName,
        AZStd::string_view cppTypeName)
        : m_typeEnum(typeEnum)
        , m_typeUuid(typeUuid)
        , m_defaultValue(defaultValue)
        , m_displayName(typeDisplayName)
        , m_cppName(cppTypeName)
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
} // namespace GraphModel
