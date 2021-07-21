/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_DATA_OVERLAY_INSTANCE_MSGS_H
#define AZCORE_DATA_OVERLAY_INSTANCE_MSGS_H

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/DataOverlay.h>
#include <AzCore/EBus/EBus.h>

namespace AZ
{
    /**
     * DataOverlayInstanceId is used to identify a specific instance of overlayed data
     */
    class DataOverlayInstanceId
    {
    public:
        DataOverlayInstanceId(const void* ptr, const Uuid& typeId)
            : m_ptr(ptr)
            , m_typeId(typeId)
        {
        }

        bool operator==(const DataOverlayInstanceId& rhs) const
        {
            return m_ptr == rhs.m_ptr && m_typeId == rhs.m_typeId;
        }

        const void* m_ptr;
        Uuid        m_typeId;
    };

    /**
     * DataOverlayInstanceBus is used to identify overlayed instances
     */
    class DataOverlayInstanceMsgs
        : public EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        typedef DataOverlayInstanceId BusIdType;
        static const EBusHandlerPolicy HandlerPolicy = EBusHandlerPolicy::Single;
        static const EBusAddressPolicy AddressPolicy = EBusAddressPolicy::ById;
        typedef AZStd::recursive_mutex MutexType;
        //////////////////////////////////////////////////////////////////////////

        virtual ~DataOverlayInstanceMsgs() {}

        virtual DataOverlayInfo GetOverlayInfo() = 0;
    };
    typedef EBus<DataOverlayInstanceMsgs> DataOverlayInstanceBus;
}   // namespace AZ

namespace AZStd
{
    template<class T>
    struct hash;

    template<>
    struct hash<AZ::DataOverlayInstanceId>
    {
        typedef AZ::DataOverlayInstanceId   argument_type;
        typedef size_t                      result_type;
        inline result_type operator()(const argument_type& value) const { return static_cast<result_type>(reinterpret_cast<ptrdiff_t>(value.m_ptr)); }
    };
}   // namespace AZStd

#endif  // AZCORE_DATA_OVERLAY_INSTANCE_MSGS_H
#pragma once
