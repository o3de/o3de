/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Uuid.h>
#include <AzToolsFramework/Debug/TraceContext.h> // Included for the AZ_ENABLE_TRACE_CONTEXT definition.
#include <AzToolsFramework/Debug/TraceContextStack.h>

namespace AzToolsFramework
{
    namespace Debug
    {
        TraceContextStack::EntryInfo::EntryInfo(EntryInfo&& rhs)
            : m_key(AZStd::move(rhs.m_key))
            , m_type(rhs.m_type)
            , m_value(rhs.m_value)
        {
        }

        TraceContextStack::EntryInfo& TraceContextStack::EntryInfo::operator=(EntryInfo&& rhs)
        {
            m_key = AZStd::move(rhs.m_key);
            m_type = rhs.m_type;
            m_value = rhs.m_value;
            return *this;
        }

#ifdef AZ_ENABLE_TRACE_CONTEXT
        size_t TraceContextStack::GetStackCount() const
        {
            return m_entries.size();
        }

        TraceContextStackInterface::ContentType TraceContextStack::GetType(size_t index) const
        {
            return index < m_entries.size() ? m_entries[index].m_type : ContentType::Undefined;
        }

        const char* TraceContextStack::GetKey(size_t index) const
        {
            return index < m_entries.size() ? m_entries[index].m_key.c_str() : "";
        }

        const char* TraceContextStack::GetStringValue(size_t index) const
        {
            if (IsValidRequest(index, ContentType::StringType))
            {
                size_t stringIndex = m_entries[index].m_value.stringValue;
                if (stringIndex < m_stringStack.size())
                {
                    return m_stringStack[stringIndex].c_str();
                }
            }
            return "";
        }

        bool TraceContextStack::GetBoolValue(size_t index) const
        {
            return IsValidRequest(index, ContentType::BoolType) ? m_entries[index].m_value.boolValue : false;
        }

        int64_t TraceContextStack::GetIntValue(size_t index) const
        {
            return IsValidRequest(index, ContentType::IntType) ? m_entries[index].m_value.intValue : 0;
        }

        uint64_t TraceContextStack::GetUIntValue(size_t index) const
        {
            return IsValidRequest(index, ContentType::UintType) ? m_entries[index].m_value.uintValue : 0;
        }

        float TraceContextStack::GetFloatValue(size_t index) const
        {
            return IsValidRequest(index, ContentType::FloatType) ? m_entries[index].m_value.floatValue : 0.0f;
        }

        double TraceContextStack::GetDoubleValue(size_t index) const
        {
            return IsValidRequest(index, ContentType::DoubleType) ? m_entries[index].m_value.doubleValue : 0.0;
        }

        const AZ::Uuid& TraceContextStack::GetUuidValue(size_t index) const
        {
            if (IsValidRequest(index, ContentType::UuidType))
            {
                size_t uuidIndex = m_entries[index].m_value.uuidValue;
                if (uuidIndex < m_uuidStack.size())
                {
                    return m_uuidStack[uuidIndex];
                }
            }

            static const AZ::Uuid nullId = AZ::Uuid::CreateNull();
            return nullId;
        }

        bool TraceContextStack::IsValidRequest(size_t index, ContentType type) const
        {
            return index < m_entries.size() && type == m_entries[index].m_type;
        }

        void TraceContextStack::PushStringEntry(const char* key, const char* value)
        {
            EntryInfo entry;
            entry.m_type = ContentType::StringType;
            entry.m_key = key;
            entry.m_value.stringValue = m_stringStack.size();
            m_stringStack.emplace_back(value);
            m_entries.push_back(AZStd::move(entry));
        }

        void TraceContextStack::PushBoolEntry(const char* key, bool value)
        {
            EntryInfo entry;
            entry.m_type = ContentType::BoolType;
            entry.m_key = key;
            entry.m_value.boolValue = value;
            m_entries.push_back(AZStd::move(entry));
        }

        void TraceContextStack::PushIntEntry(const char* key, int64_t value)
        {
            EntryInfo entry;
            entry.m_type = ContentType::IntType;
            entry.m_key = key;
            entry.m_value.intValue = value;
            m_entries.push_back(AZStd::move(entry));
        }

        void TraceContextStack::PushUintEntry(const char* key, uint64_t value)
        {
            EntryInfo entry;
            entry.m_type = ContentType::UintType;
            entry.m_key = key;
            entry.m_value.uintValue = value;
            m_entries.push_back(AZStd::move(entry));
        }

        void TraceContextStack::PushFloatEntry(const char* key, float value)
        {
            EntryInfo entry;
            entry.m_type = ContentType::FloatType;
            entry.m_key = key;
            entry.m_value.floatValue = value;
            m_entries.push_back(AZStd::move(entry));
        }

        void TraceContextStack::PushDoubleEntry(const char* key, double value)
        {
            EntryInfo entry;
            entry.m_type = ContentType::DoubleType;
            entry.m_key = key;
            entry.m_value.doubleValue = value;
            m_entries.push_back(AZStd::move(entry));
        }

        void TraceContextStack::PushUuidEntry(const char* key, const AZ::Uuid* value)
        {
            EntryInfo entry;
            entry.m_type = ContentType::UuidType;
            entry.m_key = key;
            entry.m_value.uuidValue = m_uuidStack.size();
            m_uuidStack.push_back(value ? *value : AZ::Uuid::CreateNull());
            m_entries.push_back(AZStd::move(entry));
        }

        void TraceContextStack::PopEntry()
        {
            if (m_entries.size() > 0)
            {
                ContentType popType = m_entries[m_entries.size() - 1].m_type;
                // Some types can't be stored in unions or would make it to big. For these
                //      situations a separate stack is maintained and the index to the entry
                //      is stored in the union instead.
                if (popType == ContentType::StringType)
                {
                    if (m_stringStack.size() > 0)
                    {
                        m_stringStack.pop_back();
                    }
                }
                else if (popType == ContentType::UuidType)
                {
                    if (m_uuidStack.size() > 0)
                    {
                        m_uuidStack.pop_back();
                    }
                }
                
                m_entries.pop_back();
            }
        }

#else // AZ_ENABLE_TRACE_CONTEXT

        size_t TraceContextStack::GetStackCount() const
        {
            return 0;
        }

        TraceContextStack::ContentType TraceContextStack::GetType(size_t /*index*/) const
        {
            return ContentType::Undefined;
        }

        const char* TraceContextStack::GetKey(size_t /*index*/) const
        {
            return "";
        }

        const char* TraceContextStack::GetStringValue(size_t /*index*/) const
        {
            return "";
        }

        bool TraceContextStack::GetBoolValue(size_t /*index*/) const
        {
            return false;
        }
        
        int64_t TraceContextStack::GetIntValue(size_t /*index*/) const 
        {
            return 0;
        }

        uint64_t TraceContextStack::GetUIntValue(size_t /*index*/) const 
        {
            return 0;
        }

        float TraceContextStack::GetFloatValue(size_t /*index*/) const 
        {
            return 0.0f;
        }

        double TraceContextStack::GetDoubleValue(size_t /*index*/) const 
        {
            return 0.0;
        }

        const AZ::Uuid& TraceContextStack::GetUuidValue(size_t /*index*/) const 
        {
            static const AZ::Uuid nullId = AZ::Uuid::CreateNull();
            return nullId;
        }

        void TraceContextStack::PushStringEntry(const char* /*key*/, const char* /*value*/)
        {
        }

        void TraceContextStack::PushBoolEntry(const char* /*key*/, bool /*value*/)
        {
        }

        void TraceContextStack::PushIntEntry(const char* /*key*/, int64_t /*value*/)
        {
        }

        void TraceContextStack::PushUintEntry(const char* /*key*/, uint64_t /*value*/)
        {
        }

        void TraceContextStack::PushFloatEntry(const char* /*key*/, float /*value*/)
        {
        }

        void TraceContextStack::PushDoubleEntry(const char* /*key*/, double /*value*/)
        {
        }

        void TraceContextStack::PushUuidEntry(const char* /*key*/, const AZ::Uuid* /*value*/)
        {
        }

        void TraceContextStack::PopEntry()
        {
        }
#endif // AZ_ENABLE_TRACE_CONTEXT
    } // Debug
} // AzToolsFramework
