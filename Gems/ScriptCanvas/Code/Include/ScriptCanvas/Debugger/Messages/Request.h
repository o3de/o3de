/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Network/IRemoteTools.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Debugger/APIArguments.h>
#include <ScriptCanvas/Debugger/API.h>

namespace ScriptCanvas
{
    namespace Debugger
    {
        namespace Message
        {
            class RequestVisitor;

            class Request
                : public AzFramework::RemoteToolsMessage
            {
            public:
                AZ_CLASS_ALLOCATOR(Request, AZ::SystemAllocator);
                AZ_RTTI(Request, "{0283335F-E3FF-4292-99BA-36A289DFED87}", AzFramework::RemoteToolsMessage);

                Request() : AzFramework::RemoteToolsMessage(k_clientRequestsMsgSlotId) {}

                virtual void Visit(RequestVisitor& visitor) = 0;
            };

            template<typename t_Tag>
            class TaggedRequest
                : public Request
            {
            public:
                AZ_CLASS_ALLOCATOR(TaggedRequest<t_Tag>, AZ::SystemAllocator);
                AZ_RTTI((TaggedRequest, "{DFFD8D88-1C50-457C-B0D0-C0640D78FB76}", t_Tag), Request);

                TaggedRequest() = default;

                void Visit(RequestVisitor& visitor) override;
            };

            using BreakRequest = TaggedRequest<BreakTag>;
            using ContinueRequest = TaggedRequest<ContinueTag>;
            using GetAvailableScriptTargets = TaggedRequest<GetAvailableScriptTargetsTag>;
            using GetActiveEntitiesRequest = TaggedRequest<GetActiveEntitiesTag>;
            using GetActiveGraphsRequest = TaggedRequest<GetActiveGraphsTag>;
            using StepOverRequest = TaggedRequest<StepOverTag>;
            
            class AddBreakpointRequest;
            class ConnectRequest;
            class DisconnectRequest;

            class AddTargetsRequest;
            class RemoveTargetsRequest;

            class StartLoggingRequest;
            class StopLoggingRequest;

            class RemoveBreakpointRequest;
            
            class RequestVisitor
            {
            public:
                virtual ~RequestVisitor() = default;

                virtual void Visit(AddBreakpointRequest& request) = 0;
                virtual void Visit(BreakRequest& request) = 0;

                virtual void Visit(ContinueRequest& request) = 0;

                virtual void Visit(AddTargetsRequest& request) = 0;
                virtual void Visit(RemoveTargetsRequest& request) = 0;

                virtual void Visit(StartLoggingRequest& request) = 0;
                virtual void Visit(StopLoggingRequest& request) = 0;

                virtual void Visit(GetAvailableScriptTargets& request) = 0;
                virtual void Visit(GetActiveEntitiesRequest& request) = 0;
                virtual void Visit(GetActiveGraphsRequest& request) = 0;
                virtual void Visit(RemoveBreakpointRequest& request) = 0;
                virtual void Visit(StepOverRequest& request) = 0;
            };
                 
            class AddBreakpointRequest final
                : public Request
            {
            public:
                AZ_CLASS_ALLOCATOR(AddBreakpointRequest, AZ::SystemAllocator);
                AZ_RTTI(AddBreakpointRequest, "{F9D606B4-47EB-4B40-BF8E-01C65208A291}", Request);

                Breakpoint m_breakpoint;

                AddBreakpointRequest() = default;

                AddBreakpointRequest(const Breakpoint& breakpoint)
                    : Request()
                    , m_breakpoint(breakpoint)
                {}

                void Visit(RequestVisitor& visitor) override { visitor.Visit(*this);  }
            };
            
            class ConnectRequest final
                : public Request
            {
            public:
                AZ_CLASS_ALLOCATOR(ConnectRequest, AZ::SystemAllocator);
                AZ_RTTI(ConnectRequest, "{8EC1A888-C853-4AE6-A053-01CCACD9F6BC}", Request);

                ScriptTarget m_target;

                ConnectRequest() = default;

                ConnectRequest(const ScriptTarget& target)
                    : m_target(target)
                {}

                void Visit(RequestVisitor&) override {}
            };

            class DisconnectRequest final
                : public Request
            {
            public:
                AZ_CLASS_ALLOCATOR(DisconnectRequest, AZ::SystemAllocator);
                AZ_RTTI(DisconnectRequest, "{CD4E75F7-277B-45FB-A95F-EB804BE1D3B4}", Request);

                DisconnectRequest() = default;

                void Visit(RequestVisitor&) override {}
            };

            class StartLoggingRequest final
                : public Request
            {
            public:
                AZ_CLASS_ALLOCATOR(StartLoggingRequest, AZ::SystemAllocator);
                AZ_RTTI(StartLoggingRequest, "{066F8954-52BF-495C-8EEE-6FF43A4822F8}", Request);

                ScriptTarget m_initialTargets;

                StartLoggingRequest() = default;

                StartLoggingRequest(const ScriptTarget& initialTargets)
                    : m_initialTargets(initialTargets)
                {
                }

                void Visit(RequestVisitor& visitor) { visitor.Visit(*this); }
            };

            class StopLoggingRequest final
                : public Request
            {
            public:
                AZ_CLASS_ALLOCATOR(StopLoggingRequest, AZ::SystemAllocator);
                AZ_RTTI(StopLoggingRequest, "{37BF039D-A7E8-4BEE-B0E9-B411F566CBB4}", Request);

                StopLoggingRequest() = default;

                void Visit(RequestVisitor& visitor) { visitor.Visit(*this); }
            };

            class AddTargetsRequest final
                : public Request
            {
            public:
                AZ_CLASS_ALLOCATOR(AddTargetsRequest, AZ::SystemAllocator);
                AZ_RTTI(AddTargetsRequest, "{7A3469C5-C562-4B11-8AB5-BB4A50FD01F0}", Request);

                ScriptTarget m_addTargets;

                AddTargetsRequest() = default;
                AddTargetsRequest(ScriptTarget& scriptTargets)
                    : m_addTargets(scriptTargets)
                {

                }

                void Visit(RequestVisitor& visitor) override { visitor.Visit(*this); }
            };

            class RemoveTargetsRequest final
                : public Request
            {
            public:
                AZ_CLASS_ALLOCATOR(RemoveTargetsRequest, AZ::SystemAllocator);
                AZ_RTTI(RemoveTargetsRequest, "{9FCC465D-EB4E-4B5B-B2DE-C4DFF0C193FC}", Request);

                ScriptTarget m_removeTargets;

                RemoveTargetsRequest() = default;
                RemoveTargetsRequest(ScriptTarget& scriptTargets)
                    : m_removeTargets(scriptTargets)
                {

                }

                void Visit(RequestVisitor& visitor) override { visitor.Visit(*this); }
            };

            class RemoveBreakpointRequest final
                : public Request
            {
            public:
                AZ_CLASS_ALLOCATOR(RemoveBreakpointRequest, AZ::SystemAllocator);
                AZ_RTTI(RemoveBreakpointRequest, "{E50ADBD5-8B36-445A-ACB4-A7E091CE06EA}", Request);

                Breakpoint m_breakpoint;

                RemoveBreakpointRequest() = default;

                RemoveBreakpointRequest(const Breakpoint& breakpoint)
                    : Request()
                    , m_breakpoint(breakpoint)
                {}

                void Visit(RequestVisitor& visitor) override { visitor.Visit(*this); }
            };
                        
            template<typename t_Tag>
            void TaggedRequest<t_Tag>::Visit(RequestVisitor& visitor)
            {
                visitor.Visit(*this);
            };
        }
    }
}
