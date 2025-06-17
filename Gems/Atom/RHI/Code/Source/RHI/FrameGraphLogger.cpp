/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/FrameGraphLogger.h>
#include <Atom/RHI/FrameGraph.h>
#include <Atom/RHI/FrameGraphAttachmentDatabase.h>
#include <Atom/RHI/ImageScopeAttachment.h>
#include <Atom/RHI/BufferScopeAttachment.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <AzCore/IO/SystemFile.h>

namespace AZ::RHI
{
    void FrameGraphLogger::Log(
        const FrameGraph& frameGraph,
        FrameSchedulerLogVerbosity logVerbosity)
    {
        if (logVerbosity == FrameSchedulerLogVerbosity::None)
        {
            return;
        }

        const FrameGraphAttachmentDatabase& attachmentDatabase = frameGraph.GetAttachmentDatabase();

        size_t scopeAttachmentCount = 0;
        for (Scope* scope : frameGraph.GetScopes())
        {
            scopeAttachmentCount += scope->GetAttachments().size();
        }

        AZ_Printf("FrameGraph", "==============================================\n");
        AZ_Printf("FrameGraph", "FrameGraph Log\n");
        AZ_Printf("FrameGraph", "\tScope Count: %d\n", frameGraph.GetScopes().size());
        AZ_Printf("FrameGraph", "\tAttachment Counts:\n");
        AZ_Printf("FrameGraph", "\t\tTotal: %d\n", attachmentDatabase.GetAttachments().size());
        AZ_Printf("FrameGraph", "\t\tTransient Images: %d\n", attachmentDatabase.GetTransientImageAttachments().size());
        AZ_Printf("FrameGraph", "\t\tTransient Buffers: %d\n", attachmentDatabase.GetTransientBufferAttachments().size());
        AZ_Printf("FrameGraph", "\t\tImported Images: %d\n", attachmentDatabase.GetImportedImageAttachments().size());
        AZ_Printf("FrameGraph", "\t\tImported Buffers: %d\n", attachmentDatabase.GetImportedBufferAttachments().size());
        AZ_Printf("FrameGraph", "\t\tImported Swapchains: %d\n", attachmentDatabase.GetSwapChainAttachments().size());
        AZ_Printf("FrameGraph", "\tScope Attachment Count: %d\n", scopeAttachmentCount);

        if (logVerbosity != FrameSchedulerLogVerbosity::Detail)
        {
            return;
        }

        const auto& attachments = attachmentDatabase.GetAttachments();
        AZ_Printf("FrameGraph", "Attachments:\n");
        for (size_t i = 0; i < attachments.size(); ++i)
        {
            FrameAttachment& attachment = *attachments[i];
            AZ_Printf("FrameGraph", "\t[Attachment]:\t%s\n", attachment.GetId().GetCStr());

            for (int deviceIndex{ 0 }; deviceIndex < RHISystemInterface::Get()->GetDeviceCount(); ++deviceIndex)
            {
                Scope* scopeFirst = attachment.GetFirstScope(deviceIndex);
                Scope* scopeLast = attachment.GetLastScope(deviceIndex);

                if (!scopeFirst)
                {
                    continue;
                }

                AZ_Printf("FrameGraph", "\t[Attachment]:\t[Device %d]\tFirst Scope: %s\n", deviceIndex, scopeFirst->GetId().GetCStr());
                AZ_Printf("FrameGraph", "\t[Attachment]:\t[Device %d]\tLast Scope: %s\n\n", deviceIndex, scopeLast->GetId().GetCStr());

                const ScopeAttachment* scopeAttachment = attachment.GetFirstScopeAttachment(deviceIndex);

                while (scopeAttachment)
                {
                    AZ_Printf("FrameGraph", "\t\t\t[Usage]:\t%s\n", scopeAttachment->GetTypeName());

                    AZ_Printf("FrameGraph", "\t\t\t[Scope]:\t%s\n\n", scopeAttachment->GetScope().GetId().GetCStr());

                    scopeAttachment = scopeAttachment->GetNext();
                }
            }
        }

        AZ_Printf("FrameGraph", "Scopes:\n");
        for (size_t i = 0; i < frameGraph.GetScopes().size(); ++i)
        {
            Scope* scope = frameGraph.GetScopes()[i];
            AZ_Printf(
                "FrameGraph", "\t[Scope]: %s\n",
                scope->GetId().GetCStr());

            if (frameGraph.GetConsumers(*scope).size())
            {
                AZ_Printf("FrameGraph", "\t\t[Dependents]:\n");
                for (Scope* dependent : frameGraph.GetConsumers(*scope))
                {
                    AZ_Printf("FrameGraph", "\t\t\t[%s]\n", dependent->GetId().GetCStr());
                }
                AZ_Printf("FrameGraph", "\n");
            }

            AZ_Printf(
                "FrameGraph", "\t\t[Estimated Item Count]: %d\n\n",
                scope->GetEstimatedItemCount());

            const auto& logAttachment = [](const ScopeAttachment* scopeAttachment)
            {
                AZ_Printf("FrameGraph", "\t\t\t[Name: %s][Type: %s]\n",
                    scopeAttachment->GetFrameAttachment().GetId().GetCStr(),
                    scopeAttachment->GetTypeName());
            };

            if (scope->GetImageAttachments().size())
            {
                AZ_Printf("FrameGraph", "\t\t[Images]:\n");
                for (const ScopeAttachment* scopeAttachment : scope->GetImageAttachments())
                {
                    logAttachment(scopeAttachment);
                }
                AZ_Printf("FrameGraph", "\n");
            }

            if (scope->GetBufferAttachments().size())
            {
                AZ_Printf("FrameGraph", "\t\t[Buffers]:\n");
                for (const ScopeAttachment* scopeAttachment : scope->GetBufferAttachments())
                {
                    logAttachment(scopeAttachment);
                }
                AZ_Printf("FrameGraph", "\n");
            }
        }

        DumpGraphVis(frameGraph);
    }

    void FrameGraphLogger::DumpGraphVis(const FrameGraph& frameGraph)
    {
        const FrameGraphAttachmentDatabase& attachmentDatabase = frameGraph.GetAttachmentDatabase();

        const char* HardwareQueueClassNames[] =
        {
            "Graphics",
            "Compute",
            "Copy"
        };

        const char* HardwareQueueClassColors[] =
        {
            "steelblue3",
            "darkolivegreen3",
            "darkorange1"
        };

        //////////////////////////////////////////////////////////////////////////
        //
        // Print raw graph
        //
        //////////////////////////////////////////////////////////////////////////

        {
            AZ::IO::SystemFile file;
            file.Open("Logs/FrameSchedulerGraphRaw.gv", AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY);

            AZStd::string outputText = "digraph {\nrankdir=LR;\n";

            for (Scope* scope : frameGraph.GetScopes())
            {
                outputText += AZStd::string::format("\t\"%s\" [shape=rectangle, style=filled, color=%s];\n",
                    scope->GetId().GetCStr(),
                    HardwareQueueClassColors[(uint32_t)scope->GetHardwareQueueClass()]);
            }

            for (Scope* producer : frameGraph.GetScopes())
            {
                for (Scope* consumer : frameGraph.GetConsumers(*producer))
                {
                    outputText += AZStd::string::format(
                        "\t\"%s\" -> \"%s\" [color=\"%s:%s;0.5\", penwidth=3.0",
                        producer->GetId().GetCStr(), consumer->GetId().GetCStr(),
                        HardwareQueueClassColors[(uint32_t)producer->GetHardwareQueueClass()],
                        HardwareQueueClassColors[(uint32_t)consumer->GetHardwareQueueClass()]);

                    if (producer->GetHardwareQueueClass() != consumer->GetHardwareQueueClass())
                    {
                        outputText += ", style=dotted";
                    }

                    outputText += "];\n";
                }
            }

            for (FrameAttachment* attachment : attachmentDatabase.GetAttachments())
            {
                outputText += AZStd::string::format("\t\"_%s\" [shape=oval, color=indigo, fontcolor=indigo];\n", attachment->GetId().GetCStr());
            }

            for (ScopeAttachment* scopeAttachmentCurr : attachmentDatabase.GetScopeAttachments())
            {
                const AttachmentId& attachmentId = scopeAttachmentCurr->GetFrameAttachment().GetId();

                if (scopeAttachmentCurr->GetPrevious())
                {
                    const ScopeAttachment* scopeAttachmentPrev = scopeAttachmentCurr->GetPrevious();

                    AZStd::string transitionName = AZStd::string::format(
                        "[%s]: [%s] -> [%s]",
                        attachmentId.GetCStr(),
                        scopeAttachmentPrev->GetTypeName(),
                        scopeAttachmentCurr->GetTypeName());

                    outputText += AZStd::string::format(
                        "\t\"%s\" -> \"%s\" [color=indigo, label=\"%s\", fontcolor=indigo",
                        scopeAttachmentPrev->GetScope().GetId().GetCStr(),
                        scopeAttachmentCurr->GetScope().GetId().GetCStr(),
                        transitionName.data());

                    if (scopeAttachmentCurr->GetScope().GetHardwareQueueClass() != scopeAttachmentPrev->GetScope().GetHardwareQueueClass())
                    {
                        outputText += ", style=dotted";
                    }

                    outputText += "];\n";
                }
                else
                {
                    outputText += AZStd::string::format(
                        "\t\"_%s\" -> \"%s\" [style=filled, color=indigo, fontcolor=indigo, label=\"[%s]: %s\"];\n",
                        attachmentId.GetCStr(),
                        scopeAttachmentCurr->GetScope().GetId().GetCStr(),
                        attachmentId.GetCStr(),
                        scopeAttachmentCurr->GetTypeName());
                }
            }

            outputText += "}\n";

            file.Write(outputText.data(), outputText.size());
            file.Close();
        }

        //////////////////////////////////////////////////////////////////////////
        //
        // Print compiled graph
        //
        //////////////////////////////////////////////////////////////////////////

        {
            AZ::IO::SystemFile file;
            file.Open("Logs/FrameSchedulerGraph.gv", AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY);

            AZStd::string outputText = "digraph {\nrankdir=LR;\n";

            for (uint32_t hardwareQueueClassIdx = 0; hardwareQueueClassIdx < HardwareQueueClassCount; ++hardwareQueueClassIdx)
            {
                outputText += AZStd::string::format("\tsubgraph %s {\n", HardwareQueueClassNames[hardwareQueueClassIdx]);
                outputText += AZStd::string::format("\t\tnode[style=filled, color=%s]\n", HardwareQueueClassColors[hardwareQueueClassIdx]);
                outputText += "\tstyle=filled;\n";
                for (Scope* scope : frameGraph.GetScopes())
                {
                    if (scope->GetHardwareQueueClass() == static_cast<HardwareQueueClass>(hardwareQueueClassIdx))
                    {
                        outputText += AZStd::string::format("\t\t\"%s\"\n", scope->GetId().GetCStr());
                    }
                }
                outputText += "\t}\n";
            }

            for (Scope* scope : frameGraph.GetScopes())
            {
                for (uint32_t hardwareQueueClassIdx = 0; hardwareQueueClassIdx < HardwareQueueClassCount; ++hardwareQueueClassIdx)
                {
                    const HardwareQueueClass hardwareQueueClass = static_cast<HardwareQueueClass>(hardwareQueueClassIdx);
                    const bool isCrossQueue = hardwareQueueClass != scope->GetHardwareQueueClass();

                    if (const Scope* consumer = scope->GetConsumerByQueue(hardwareQueueClass))
                    {
                        outputText += AZStd::string::format("\t\"%s\" -> \"%s\"", scope->GetId().GetCStr(), consumer->GetId().GetCStr());
                        if (isCrossQueue)
                        {
                            outputText += "[style=dotted, color=blue]";
                        }
                        outputText += ";\n";
                    }
                }
            }

            outputText += "}\n";

            file.Write(outputText.data(), outputText.size());
            file.Close();
        }

        //////////////////////////////////////////////////////////////////////////
        //
        // Print flat list
        //
        //////////////////////////////////////////////////////////////////////////

        {
            AZ::IO::SystemFile file;
            file.Open("Logs/FrameSchedulerGraphFlat.gv", AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY);

            AZStd::string outputText = "digraph {\nrankdir=LR;\n";

            for (uint32_t hardwareQueueClassIdx = 0; hardwareQueueClassIdx < HardwareQueueClassCount; ++hardwareQueueClassIdx)
            {
                outputText += AZStd::string::format("\tsubgraph %s {\n", HardwareQueueClassNames[hardwareQueueClassIdx]);
                outputText += AZStd::string::format("\t\tnode[style=filled, color=%s]\n", HardwareQueueClassColors[hardwareQueueClassIdx]);
                outputText += "\tstyle=filled;\n";
                for (Scope* scope : frameGraph.GetScopes())
                {
                    if (scope->GetHardwareQueueClass() == static_cast<HardwareQueueClass>(hardwareQueueClassIdx))
                    {
                        outputText += AZStd::string::format("\t\t\"%s\"\n", scope->GetId().GetCStr());
                    }
                }
                outputText += "\t}\n";
            }

            Scope* scopePrev = nullptr;
            for (Scope* scope : frameGraph.GetScopes())
            {
                if (scopePrev)
                {
                    outputText += AZStd::string::format("\t\"%s\" -> \"%s\"\n;", scopePrev->GetId().GetCStr(), scope->GetId().GetCStr());
                }
                scopePrev = scope;
            }

            outputText += "}\n";

            file.Write(outputText.data(), outputText.size());
            file.Close();
        }
    }
}
