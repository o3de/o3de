/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef LUAEDITOR_LUATARGETCONTEXTTRACKERMESSAGES_H
#define LUAEDITOR_LUATARGETCONTEXTTRACKERMESSAGES_H

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>

#pragma once

namespace LUAEditor
{
    // messages going FROM the lua Context TO anyone interested in target contexts

    //class LUATargetContextTrackerMessages : public AZ::EBusTraits
    //{
    //public:
    //  //////////////////////////////////////////////////////////////////////////
    //  // Bus configuration
    //  static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // we have one bus that we always broadcast to
    //  static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Multiple; // we can have multiple listeners
    //  //////////////////////////////////////////////////////////////////////////
    //  typedef AZ::EBus<LUATargetContextTrackerMessages> Bus;
    //  typedef Bus::Handler Handler;

    //  virtual ~LUATargetContextTrackerMessages() {}
    //};

    // messages going TO the lua Context FROM anyone interested in retrieving or setting target context info

    class LUATargetContextRequestMessages
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // Bus configuration
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single; // we have one bus that we always broadcast to
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Single; // we only have one listener
        //////////////////////////////////////////////////////////////////////////
        typedef AZ::EBus<LUATargetContextRequestMessages> Bus;
        typedef Bus::Handler Handler;

        virtual const AZStd::vector<AZStd::string> RequestTargetContexts() = 0;
        virtual const AZStd::string RequestCurrentTargetContext() = 0;
        virtual void SetCurrentTargetContext(AZStd::string& contextName) = 0;

        virtual ~LUATargetContextRequestMessages() {}
    };
}

#endif//LUAEDITOR_LUATARGETCONTEXTTRACKERMESSAGES_H
