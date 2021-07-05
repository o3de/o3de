/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Debugger/Messages/Request.h>

namespace ScriptCanvas
{
    namespace Debugger
    {
        void ReflectRequests(AZ::ReflectContext* context)
        {
            using namespace Message;

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<Request, AzFramework::TmMsg>()
                    ;

                serializeContext->Class<AddBreakpointRequest, Request>()
                    ->Field("breakpoint", &AddBreakpointRequest::m_breakpoint)
                    ;

                serializeContext->Class<BreakRequest, Request>()
                    ;

                serializeContext->Class<ConnectRequest, Request>()
                    ->Field("target", &ConnectRequest::m_target)
                    ;

                serializeContext->Class<DisconnectRequest, Request>()
                    ;

                serializeContext->Class<ContinueRequest, Request>()
                    ;

                serializeContext->Class<AddTargetsRequest, Request>()
                    ->Field("Targets", &AddTargetsRequest::m_addTargets)
                    ;

                serializeContext->Class<RemoveTargetsRequest, Request>()
                    ->Field("Targets", &RemoveTargetsRequest::m_removeTargets)
                    ;

                serializeContext->Class<StartLoggingRequest, Request>()
                    ->Field("Targets", &StartLoggingRequest::m_initialTargets)
                    ;

                serializeContext->Class<StopLoggingRequest, Request>()
                    ;

                serializeContext->Class<GetAvailableScriptTargets, Request>()
                    ;
                
                serializeContext->Class<GetActiveEntitiesRequest, Request>()
                    ;

                serializeContext->Class<GetActiveGraphsRequest, Request>()
                    ;

                serializeContext->Class<RemoveBreakpointRequest, Request>()
                    ->Field("breakpoint", &RemoveBreakpointRequest::m_breakpoint)
                    ;

                serializeContext->Class<StepOverRequest, Request>()
                    ;
            }
        }
    }
} 
