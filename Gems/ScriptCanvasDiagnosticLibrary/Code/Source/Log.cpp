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

#include "precompiled.h"
#include "Log.h"

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Debug
        {
            void Log::OnInputSignal([[maybe_unused]] const SlotId& slotId)
            {
                auto valueDatum = FindDatum(GetSlotId("Value"));
                if (!valueDatum)
                {
                    return;
                }

                AZStd::string text;
                if (!valueDatum->Empty())
                {
                    valueDatum->ToString(text);
                }

                if (!text.empty())
                {
                    AZ_TracePrintf("Script Canvas", "%s\n", text.c_str());
                    LogNotificationBus::Event(GetOwningScriptCanvasId(), &LogNotifications::LogMessage, text);
                }

                SignalOutput(GetSlotId("Out"));
            }
        }
    }
}
