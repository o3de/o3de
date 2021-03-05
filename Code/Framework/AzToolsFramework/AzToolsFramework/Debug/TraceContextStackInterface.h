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
#pragma once

#include <stdint.h>
#include <AzCore/PlatformDef.h>

namespace AZ
{
    struct Uuid;
}

namespace AzToolsFramework
{
    namespace Debug
    {
        class TraceContextStackInterface
        {
        public:
            enum class ContentType
            {
                StringType,
                BoolType,
                IntType,
                UintType,
                FloatType,
                DoubleType,
                UuidType,

                Undefined
            };

            virtual ~TraceContextStackInterface() = 0;

            virtual size_t GetStackCount() const = 0;

            virtual ContentType GetType(size_t index) const = 0;
            virtual const char* GetKey(size_t index) const = 0;
            
            virtual const char* GetStringValue(size_t index) const = 0;
            virtual bool GetBoolValue(size_t index) const = 0;
            virtual int64_t GetIntValue(size_t index) const = 0;
            virtual uint64_t GetUIntValue(size_t index) const = 0;
            virtual float GetFloatValue(size_t index) const = 0;
            virtual double GetDoubleValue(size_t index) const = 0;
            virtual const AZ::Uuid& GetUuidValue(size_t index) const = 0;
        };

        inline TraceContextStackInterface::~TraceContextStackInterface() = default;
    } // Debug
} // AzToolsFramework
