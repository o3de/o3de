/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Network/IRemoteTools.h>
#include <Core/ExecutionNotificationsBus.h>
#include <ScriptCanvas/Debugger/APIArguments.h>
#include <ScriptCanvas/Debugger/API.h>

namespace ScriptCanvas
{
    namespace Debugger
    {
        namespace Message
        {
            class NotificationVisitor;

            class Notification
                : public AzFramework::RemoteToolsMessage
            {
            public:
                AZ_CLASS_ALLOCATOR(Notification, AZ::SystemAllocator);
                AZ_RTTI(Notification, "{2FBEC565-7F5F-435E-8BC6-DD17CC1FABE7}", AzFramework::RemoteToolsMessage);

                Notification() : AzFramework::RemoteToolsMessage(k_serviceNotificationsMsgSlotId) {}

                virtual void Visit(NotificationVisitor& visitor) = 0;
            };

            template<typename t_Payload>
            class NotificationPayload 
                : public Notification
            {
            public:
                AZ_CLASS_ALLOCATOR(NotificationPayload<t_Payload>, AZ::SystemAllocator);
                AZ_RTTI((NotificationPayload, "{AC9FC9F9-C660-43DC-BCB8-0CD390432ED1}", t_Payload), Notification);

                t_Payload m_payload;

                NotificationPayload() = default;

                NotificationPayload(const t_Payload& payload)
                    : m_payload(payload)
                {}

                void Visit(NotificationVisitor& visitor) override;

                static void Reflect(AZ::ReflectContext* context)
                {
                    using namespace Message;

                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
                    {
                        serializeContext->Class<NotificationPayload<t_Payload>, Notification>()
                            ->Field("payload", &NotificationPayload<t_Payload>::m_payload)
                            ;
                    }
                }
            };
            
            class BreakpointAdded;
            class BreakpointHit;
            class Connected;
            class Disconnected;
            class Continued;
            class SignaledInput;
            class SignaledOutput;
            class VariableChanged;
            
            using ActiveEntitiesResult = NotificationPayload<ActiveEntityStatusMap>;
            using ActiveGraphsResult = NotificationPayload<ActiveGraphStatusMap>;
            using AvailableScriptTargetsResult = NotificationPayload<ActiveEntitiesAndGraphs>;
            using GraphActivated = NotificationPayload<GraphActivation>;
            using GraphDeactivated = NotificationPayload<GraphDeactivation>;
            using AnnotateNode = NotificationPayload<AnnotateNodeSignal>;

            class NotificationVisitor
            {
            public:
                virtual ~NotificationVisitor() = default;
                
                virtual void Visit(ActiveEntitiesResult& notification) = 0;
                virtual void Visit(ActiveGraphsResult& notification) = 0;
                virtual void Visit(AnnotateNode& notification) = 0;
                virtual void Visit(AvailableScriptTargetsResult& notification) = 0;
                virtual void Visit(BreakpointHit& notification) = 0;
                virtual void Visit(BreakpointAdded& notification) = 0;
                virtual void Visit(Connected& notification) = 0;
                virtual void Visit(Disconnected& notification) = 0;
                virtual void Visit(Continued& notification) = 0;
                virtual void Visit(GraphActivated& notification) = 0;
                virtual void Visit(GraphDeactivated& notification) = 0;
                virtual void Visit(SignaledInput& notification) = 0;
                virtual void Visit(SignaledOutput& notification) = 0;
                virtual void Visit(VariableChanged& notification) = 0;
            };

            class BreakpointAdded final
                : public Notification
            {
            public:
                AZ_CLASS_ALLOCATOR(BreakpointAdded, AZ::SystemAllocator);
                AZ_RTTI(BreakpointAdded, "{D1F1D760-57B5-42A5-B74A-B7DEC37C320E}", Notification);

                Breakpoint m_breakpoint;

                BreakpointAdded() = default;

                BreakpointAdded(const Breakpoint& breakpoint)
                    : Notification()
                    , m_breakpoint(breakpoint)
                {}

                void Visit(NotificationVisitor& visitor) override { visitor.Visit(*this); }
            };

            class BreakpointHit final
                : public Notification
            {
            public:
                AZ_CLASS_ALLOCATOR(BreakpointHit, AZ::SystemAllocator);
                AZ_RTTI(BreakpointHit, "{CF28546A-7A3F-46E0-8A96-39555F8684F2}", Notification);

                Breakpoint m_breakpoint;

                BreakpointHit() = default;
                
                BreakpointHit(const Breakpoint& breakpoint)
                    : Notification()
                    , m_breakpoint(breakpoint)
                {}

                void Visit(NotificationVisitor& visitor) override { visitor.Visit(*this); }
            };
                            
            class Connected final
                : public Notification
            {
            public:
                AZ_CLASS_ALLOCATOR(Connected, AZ::SystemAllocator);
                AZ_RTTI(Connected, "{5AED7FF5-FEA3-4F24-A5D6-25A2554AA018}", Notification);

                Target m_target;

                Connected() = default;

                Connected(const Target& target)
                    : m_target(target)
                {}

                void Visit(NotificationVisitor& visitor) override { visitor.Visit(*this); }
            };

            class Disconnected final
                : public Notification
            {
            public:
                AZ_CLASS_ALLOCATOR(Disconnected, AZ::SystemAllocator);
                AZ_RTTI(Disconnected, "{9A2280F2-0D2F-41E6-A0DB-6DBC65D039E3}", Notification);

                Disconnected() = default;

                void Visit(NotificationVisitor& visitor) override { visitor.Visit(*this); }
            };
            
            class Continued
                : public Notification
            {
            public:
                AZ_CLASS_ALLOCATOR(Continued, AZ::SystemAllocator);
                AZ_RTTI(Continued, "{C3EBD826-115A-4EBC-8390-1FC8E4405395}", Notification);

                virtual void Visit(NotificationVisitor& visitor) { visitor.Visit(*this); }
            };

            class SignaledInput
                : public Notification
            {
            public:
                AZ_CLASS_ALLOCATOR(SignaledInput, AZ::SystemAllocator);
                AZ_RTTI(SignaledInput, "{1FFD4CF1-4D3A-4FA7-8D57-5C178EFE9CA7}", Notification);

                InputSignal m_signal;
                
                SignaledInput() = default;

                SignaledInput(const InputSignal& nodeSignalInput)
                    : m_signal(nodeSignalInput)
                {}

                virtual void Visit(NotificationVisitor& visitor) { visitor.Visit(*this); }
            };

            class SignaledOutput
                : public Notification
            {
            public:
                AZ_CLASS_ALLOCATOR(SignaledOutput, AZ::SystemAllocator);
                AZ_RTTI(SignaledOutput, "{63805157-F333-4999-8FE2-93E3F71C23F7}", Notification);

                OutputSignal m_signal;

                SignaledOutput() = default;

                SignaledOutput(const OutputSignal& signal)
                    : m_signal(signal)
                {}

                virtual void Visit(NotificationVisitor& visitor) { visitor.Visit(*this); }
            };
            
            class VariableChanged
                : public Notification
            {
            public:
                AZ_CLASS_ALLOCATOR(VariableChanged, AZ::SystemAllocator);
                AZ_RTTI(VariableChanged, "{86D554CF-D998-4AB6-B528-10273584A301}", Notification);

                VariableChange m_variableChange;

                VariableChanged() = default;

                VariableChanged(const VariableChange& change)
                    : m_variableChange(change)
                {}

                virtual void Visit(NotificationVisitor& visitor) { visitor.Visit(*this); }
            };            

            template<typename t_Payload>
            void NotificationPayload<t_Payload>::Visit(NotificationVisitor& visitor)
            {
                visitor.Visit(*this);
            }
        }
    }
}
