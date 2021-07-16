/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GRIDMATE_TYPES_H
#define GRIDMATE_TYPES_H 1

/// \file types.h

#include <AzCore/base.h>
#include <AzCore/RTTI/TypeInfo.h>

#include <AzCore/std/chrono/chrono.h>

#include <GridMate/Memory.h>

namespace AZStd
{
    template<class T>
    struct IntrusivePtrCountPolicy;
}

namespace GridMate
{
    struct ConnectionCommon
    {
        ConnectionCommon()
            : m_handshakeData(NULL) {}
        void*   m_handshakeData;
    };
    /// Carrier connection identifier.
    typedef ConnectionCommon*  ConnectionID;

    static const ConnectionID AllConnections = reinterpret_cast<const ConnectionID>(-1);
    static const ConnectionID InvalidConnectionID = NULL;

    typedef AZ::u32 VersionType;

    enum class EndianType
    {
        BigEndian,
        LittleEndian,
        IgnoreEndian,
    };

    enum NatType : AZ::u8
    {
        NAT_UNKNOWN = 0,
        NAT_OPEN,
        NAT_MODERATE,
        NAT_STRICT,
    };

    typedef AZStd::chrono::system_clock::time_point TimeStamp;

    /**
    * Different on line service types.
    * \note if a platform supports multiple you can switch them, but you need to
    * first stop all currently running services.
    */
    enum ServiceType : int
    {
        ST_LAN,
        ST_PROVO,
        ST_SALEM,
        ST_STEAM,
        ST_JASPER,
        ST_MAX // MUST BE LAST
    };

    typedef AZ::u32 GridMateServiceId;

    /**
     * Base class for reference counted objects, which are NOT
     * thread safe.
     */
    class ReferenceCounted
    {
        template<class T>
        friend struct AZStd::IntrusivePtrCountPolicy;

        mutable unsigned int    m_refCount;

        void    add_ref()       { ++m_refCount; }
        void    release()
        {
            AZ_Assert(m_refCount > 0, "Reference count logic error, trying to remove reference when refcount is 0");
            if (--m_refCount == 0)
            {
                delete this;
            }
        }

    protected:
        ReferenceCounted()
            : m_refCount(0)  {}
        virtual ~ReferenceCounted()         {}
    };
}

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(GridMate::ServiceType, "{7DA6C7AF-3EA3-49AD-894D-53046D7965B2}");
}

#endif // GRIDMATE_TYPES_H



