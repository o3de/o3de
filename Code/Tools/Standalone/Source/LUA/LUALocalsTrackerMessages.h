/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef LUAEDITOR_LUALOCALSTRACKERMESSAGES_H
#define LUAEDITOR_LUALOCALSTRACKERMESSAGES_H

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>

#pragma once

namespace LUAEditor
{
    typedef AZStd::vector<AZStd::string> LocalsList;

    // messages going FROM the lua Context TO anyone interested in local variables updates (aka the locals panel)

    class LUALocalsTrackerMessages
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // Bus configuration
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // we have one bus that we always broadcast to
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Multiple; // we can have multiple listeners
        //////////////////////////////////////////////////////////////////////////
        typedef AZ::EBus<LUALocalsTrackerMessages> Bus;
        typedef Bus::Handler Handler;

        virtual void LocalsUpdate(const AZStd::vector<AZStd::string>& vars) = 0;
        virtual void LocalsClear() = 0;

        virtual ~LUALocalsTrackerMessages() {}
    };

    // messages going TO the lua Context FROM anyone interested in retrieving local variables info

    //class LUALocalsRequestMessages : public AZ::EBusTraits
    //{
    //public:
    //  //////////////////////////////////////////////////////////////////////////
    //  // Bus configuration
    //  static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // we have one bus that we always broadcast to
    //  static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Single; // we only have one listener
    //  //////////////////////////////////////////////////////////////////////////
    //  typedef AZ::EBus<LUALocalsRequestMessages> Bus;
    //  typedef Bus::Handler Handler;

    //  virtual void RequestLocalsClicked(const AZStd::string& name) = 0;

    //  virtual ~LUALocalsRequestMessages() {}
    //};
}

#endif//LUAEDITOR_LUALOCALSTRACKERMESSAGES_H
