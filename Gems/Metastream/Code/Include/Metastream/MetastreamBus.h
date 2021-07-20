/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <Cry_Math.h>

namespace Metastream
{
    class MetastreamRequests
        : public AZ::EBusTraits
    {
    public:
        ////////////////////////////////////////////////////////////////////////////
        // EBusTraits
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        ////////////////////////////////////////////////////////////////////////////
        
        // Public functions
        virtual void AddStringToCache(const char* table, const char* key, const char* value) = 0;
        virtual void AddBoolToCache(const char* table, const char* key, bool value) = 0;
        virtual void AddVec3ToCache(const char* table, const char* key, const Vec3 & value) = 0;
        virtual void AddDoubleToCache(const char* table, const char* key, double value) = 0;
        virtual void AddUnsigned64ToCache(const char* table, const char* key, AZ::u64 value) = 0;
        virtual void AddSigned64ToCache(const char* table, const char* key, AZ::s64 value) = 0;
        virtual void AddEntityIdToCache(const char* table, const char* key, AZ::EntityId& value) {
            AddUnsigned64ToCache(table, key, static_cast<AZ::u64>(value));
        };

        virtual void AddArrayToCache(const char* table, const char* key, const char* arrayName) = 0;
        virtual void AddObjectToCache(const char* table, const char* key, const char* objectName) = 0;

        virtual void AddStringToArray(const char* table, const char* arrayName, const char* value) = 0;
        virtual void AddBoolToArray(const char* table, const char* arrayName, bool value) = 0;
        virtual void AddVec3ToArray(const char* table, const char* arrayName, const Vec3 & value) = 0;
        virtual void AddDoubleToArray(const char* table, const char* arrayName, double value) = 0;
        virtual void AddUnsigned64ToArray(const char* table, const char* arrayName, AZ::u64 value) = 0;
        virtual void AddSigned64ToArray(const char* table, const char* arrayName, AZ::s64 value) = 0;
        virtual void AddEntityIdToArray(const char* table, const char* arrayName, AZ::EntityId& value) {
            AddUnsigned64ToArray(table, arrayName, static_cast<AZ::u64>(value));
        };

        virtual void AddArrayToObject(const char* table, const char* destObjectName, const char *key, const char *srcArrayName) = 0;
        virtual void AddObjectToObject(const char* table, const char* destObjectName, const char *key, const char* sourceObjectName) = 0;
        virtual void AddObjectToArray(const char* table, const char* destArrayName, const char* sourceObjectName) = 0;
        virtual void AddStringToObject(const char* table, const char* objectName, const char* key, const char* value) = 0;
        virtual void AddBoolToObject(const char* table, const char* objectName, const char* key, bool value) = 0;
        virtual void AddVec3ToObject(const char* table, const char* objectName, const char* key, const Vec3 & value) = 0;
        virtual void AddDoubleToObject(const char* table, const char* objectName, const char* key, double value) = 0;
        virtual void AddUnsigned64ToObject(const char* table, const char* objectName, const char* key, AZ::u64 value) = 0;
        virtual void AddSigned64ToObject(const char* table, const char* objectName, const char* key, AZ::s64 value) = 0;
        virtual void AddEntityIdToObject(const char* table, const char* objectName, const char* key, AZ::EntityId& value) {
            AddUnsigned64ToObject(table, objectName, key, static_cast<AZ::u64>(value));
        };
                
        virtual bool StartHTTPServer() = 0;
        virtual void StopHTTPServer() = 0;
    };

    using MetastreamRequestBus = AZ::EBus<MetastreamRequests>;

} // namespace Metastream
