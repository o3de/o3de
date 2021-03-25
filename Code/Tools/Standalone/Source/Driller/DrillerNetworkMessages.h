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

#ifndef DRILLER_DRILLERNETWORKMESSAGES_H
#define DRILLER_DRILLERNETWORKMESSAGES_H

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>

#include <AzCore/std/containers/list.h>

#pragma once

namespace Driller
{
    // forward declarations
    class Aggregator;

    // messages going FROM the Driller Network TO anyone interested in watching data (ie. driller main window)

    class DrillerNetworkMessages
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // Bus configuration
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById; // components have an actual ID that they report back on
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Multiple; // we can have multiple listeners
        //////////////////////////////////////////////////////////////////////////
        typedef int BusIdType;
        typedef AZ::EBus<DrillerNetworkMessages> Bus;
        typedef Bus::Handler Handler;

        typedef AZStd::vector<Aggregator*> AggregatorList;

        virtual void ConnectedToNetwork() = 0;
        virtual void NewAggregatorList(AggregatorList& theList) = 0;
        virtual void AddAggregator(Aggregator& theAggregator) = 0;
        virtual void DiscardAggregators() = 0;
        virtual void DisconnectedFromNetwork() = 0;
        virtual void EndFrame(int frame) = 0;
        virtual void NewAggregatorsAvailable() = 0;

        virtual ~DrillerNetworkMessages() {}
    };
}

#endif//DRILLER_DRILLERNETWORKMESSAGES_H
