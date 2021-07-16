/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

    }
} 
