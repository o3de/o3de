/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
