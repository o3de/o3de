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
    } // namespace Debugger
} // namespace ScriptCanvas
