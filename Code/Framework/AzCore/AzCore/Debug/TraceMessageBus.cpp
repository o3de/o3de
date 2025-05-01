/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Debug/TraceMessageBus.h>

AZ_DECLARE_EBUS_EXTERN_SINGLE_ADDRESS(AZCORE_API, AZ::Debug::TraceMessageEvents);
template class AZ::EBus<AZ::Debug::TraceMessageEvents>;
template class AZ::Internal::NonIdHandler<
    AZ::Debug::TraceMessageEvents,
    AZ::Debug::TraceMessageEvents,
    AZ::EBus<AZ::Debug::TraceMessageEvents, AZ::Debug::TraceMessageEvents>::BusesContainer>;

// 5>AzCore.Static.lib(TraceMessageBus.obj) : error LNK2005: "public: __cdecl
// AZ::Internal::NonIdHandler<class AZ::Debug::TraceMessageEvents,
//                            class AZ::Debug::TraceMessageEvents,
//                            struct AZ::Internal::EBusContainer<class AZ::Debug::TraceMessageEvents,
//                                                               class AZ::Debug::TraceMessageEvents,
//                                                               0,
//                                                               1> >::
//               NonIdHandler<class AZ::Debug::TraceMessageEvents,
//                            class AZ::Debug::TraceMessageEvents,
//                            struct AZ::Internal::EBusContainer<class AZ::Debug::TraceMessageEvents,
//                                                               class AZ::Debug::TraceMessageEvents,
//                                                               0,
//                                                               1> >(void)"
// (??0?$NonIdHandler@VTraceMessageEvents@Debug@AZ@@V123@U?$EBusContainer@VTraceMessageEvents@Debug@AZ@@V123@$0A@$00@Internal@3@@Internal@AZ@@QEAA@XZ) already defined in AzFramework.lib(AzFramework.dll)
