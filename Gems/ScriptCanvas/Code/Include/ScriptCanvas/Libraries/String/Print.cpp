/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Print.h"

#include <ScriptCanvas/Execution/RuntimeBus.h>
#include <ScriptCanvas/Execution/ExecutionBus.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace String
        {
            void Print::OnInputSignal([[maybe_unused]] const SlotId& slotId)
            {
#if !defined(PERFORMANCE_BUILD) && !defined(_RELEASE) 
                AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::Print::OnInputSignal");

                AZStd::string text = ProcessFormat();

                AZ_TracePrintf("Script Canvas", "%s\n", text.c_str());
                LogNotificationBus::Event(GetOwningScriptCanvasId(), &LogNotifications::LogMessage, text);

                AZ::EntityId assetNodeId{};
                ScriptCanvas::RuntimeRequestBus::EventResult(assetNodeId, GetOwningScriptCanvasId(), &ScriptCanvas::RuntimeRequests::FindAssetNodeIdByRuntimeNodeId, GetEntityId());

                SC_EXECUTION_TRACE_ANNOTATE_NODE((*this), (AnnotateNodeSignal(CreateGraphInfo(GetOwningScriptCanvasId(), GetGraphIdentifier()), AnnotateNodeSignal::AnnotationLevel::Info, text, AZ::NamedEntityId(assetNodeId, GetNodeName()))));
#endif
                SignalOutput(GetSlotId("Out"));

            }
        }
    }
}
