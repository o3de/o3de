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

#include <Debugger/Messages/Notify.h>

namespace ScriptCanvas
{
    namespace Debugger
    {
        

        void ReflectNotifications(AZ::ReflectContext* context)
        {
            using namespace Message;

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<Notification, AzFramework::TmMsg>()
                    ;

                ActiveEntitiesResult::Reflect(context);
                ActiveGraphsResult::Reflect(context);
                AvailableScriptTargetsResult::Reflect(context);
                GraphActivated::Reflect(context);
                GraphDeactivated::Reflect(context);
                AnnotateNode::Reflect(context);
                SignaledDataOutput::Reflect(context);

                serializeContext->Class<BreakpointAdded, Notification>()
                    ->Field("breakpoint", &BreakpointAdded::m_breakpoint)
                    ;

                serializeContext->Class<BreakpointHit, Notification>()
                    ->Field("breakpoint", &BreakpointHit::m_breakpoint)
                    ;
                
                serializeContext->Class<Connected, Notification>()
                    ->Field("target", &Connected::m_target)
                    ;

                serializeContext->Class<Disconnected, Notification>()
                    ;

                serializeContext->Class<SignaledInput, Notification>()
                    ->Field("signal", &SignaledInput::m_signal)
                    ;

                serializeContext->Class<SignaledOutput, Notification>()
                    ->Field("signal", &SignaledOutput::m_signal)
                    ;

                serializeContext->Class<VariableChanged, Notification>()
                    ->Field("variableChange", &VariableChanged::m_variableChange)
                    ;
            }
        }

    } // namespace Debugger
} // namespace ScriptCanvas
