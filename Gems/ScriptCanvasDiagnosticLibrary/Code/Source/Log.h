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

#pragma once

#include <ScriptCanvas/CodeGen/CodeGen.h>
#include <ScriptCanvas/Core/Node.h>

#include <Source/Log.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Debug
        {
            class Log
                : public Node
            {
            public:
                ScriptCanvas_Node(Log,
                    ScriptCanvas_Node::Name("Log", "Logs the provided text in the debug console.")
                    ScriptCanvas_Node::Uuid("{6E100241-A738-4A8B-83C2-0FD0F5A44FDB}")
                    ScriptCanvas_Node::Icon("Editor/Icons/ScriptCanvas/Log.png")
                    ScriptCanvas_Node::Category("Utilities/Debug")
                    ScriptCanvas_Node::Version(0)
                    ScriptCanvas_Node::Deprecated("This node has been deprecated, use the Print node instead.")
                    ScriptCanvas_Node::EditAttributes(AZ::Script::Attributes::ExcludeFrom(AZ::Script::Attributes::ExcludeFlags::All))
                );

                void OnInputSignal(const SlotId& slotId) override;

                // Inputs
                ScriptCanvas_In(ScriptCanvas_In::Name("In", "Input signal"));

                // Outputs
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out", ""));

            private:

                ScriptCanvas_Property(AZStd::string,
                    ScriptCanvas_Property::Name("Value", "The value to log")
                    ScriptCanvas_Property::Input
                    );
            };
        }
    }
}
