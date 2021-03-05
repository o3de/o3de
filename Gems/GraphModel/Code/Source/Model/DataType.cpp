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
    {}

    DataType::DataType(Enum typeEnum, AZ::Uuid typeUuid, AZStd::any defaultValue, AZStd::string_view typeDisplayName, AZStd::string_view cppTypeName)
        : m_typeEnum(typeEnum)
        , m_typeUuid(typeUuid)
        , m_defaultValue(defaultValue)
        , m_displayName(typeDisplayName)
        , m_cppName(cppTypeName)
    {}

    DataType::DataType(const DataType& other)
        : m_typeEnum(other.m_typeEnum)
        , m_typeUuid(other.m_typeUuid)
        , m_defaultValue(other.m_defaultValue)
        , m_displayName(other.m_displayName)
        , m_cppName(other.m_cppName)
    {}

    bool DataType::IsValid() const
    {
        return ENUM_INVALID != m_typeEnum && !m_typeUuid.IsNull();
    }

    AZ::Uuid DataType::GetTypeUuid() const
    {
        return m_typeUuid;
    }

    AZStd::string DataType::GetTypeUuidString() const
    {
        return GetTypeUuid().ToString<AZStd::string>();
    }

    bool DataType::operator==(const DataType& other) const
    {
        return  m_typeEnum != ENUM_INVALID &&
                other.m_typeEnum != ENUM_INVALID &&
                m_typeEnum == other.m_typeEnum;
    }

    bool DataType::operator!=(const DataType& other) const
    {
        return  !(*this == other);
    }

    AZStd::any DataType::GetDefaultValue() const
    {
        return m_defaultValue;
    }

    AZStd::string DataType::GetDisplayName() const
    {
        return m_displayName;
    }

    AZStd::string DataType::GetCppName() const
    {
        return m_cppName;
    }

} // namespace GraphModel