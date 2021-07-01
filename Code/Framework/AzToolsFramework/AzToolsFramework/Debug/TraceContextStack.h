/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/Debug/TraceContextStackInterface.h>

namespace AzToolsFramework
{
    namespace Debug
    {
        class TraceContextStack : public TraceContextStackInterface
        {
        public:
            ~TraceContextStack() override = default;

            size_t GetStackCount() const override;

            ContentType GetType(size_t index) const override;
            const char* GetKey(size_t index) const override;
            
            const char* GetStringValue(size_t index) const override;
            bool GetBoolValue(size_t index) const override;
            int64_t GetIntValue(size_t index) const override;
            uint64_t GetUIntValue(size_t index) const override;
            float GetFloatValue(size_t index) const override;
            double GetDoubleValue(size_t index) const override;
            const AZ::Uuid& GetUuidValue(size_t index) const override;
            
            void PushStringEntry(const char* key, const char* value);
            void PushBoolEntry(const char* key, bool value);
            void PushIntEntry(const char* key, int64_t value);
            void PushUintEntry(const char* key, uint64_t value);
            void PushFloatEntry(const char* key, float value);
            void PushDoubleEntry(const char* key, double value);
            void PushUuidEntry(const char* key, const AZ::Uuid* value);
            void PopEntry();

        protected:
            inline bool IsValidRequest(size_t index, ContentType type) const;

            struct EntryInfo
            {
                EntryInfo() = default;
                EntryInfo(const EntryInfo& rhs) = default;
                EntryInfo(EntryInfo&& rhs);

                EntryInfo& operator=(const EntryInfo& rhs) = default;
                EntryInfo& operator=(EntryInfo&& rhs);

                AZStd::string m_key;
                ContentType m_type;
                union
                {
                    size_t stringValue;         // Index into m_stringStack
                    bool boolValue;
                    int64_t intValue;
                    uint64_t uintValue;
                    float floatValue;
                    double doubleValue;
                    size_t uuidValue;           // Index into m_uuidStack
                } m_value;
            };

            AZStd::vector<AZStd::string> m_stringStack;
            AZStd::vector<AZ::Uuid> m_uuidStack;
            AZStd::vector<EntryInfo> m_entries;
        };
    } // Debug
} // AzToolsFramework
