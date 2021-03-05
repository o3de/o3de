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
#include <ScriptCanvas/Internal/Nodes/StringFormatted.h>

#include <Include/ScriptCanvas/Libraries/String/Format.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace String
        {
            //! A String formatting class that produces a data output based on the specified string format and
            //! input values.
            class Format
                : public Internal::StringFormatted
            {
            public:
                ScriptCanvas_Node(Format,
                    ScriptCanvas_Node::Name("Build String", "Formats and creates a string from the provided text.\nAny word within {} will create a data pin on this node.")
                    ScriptCanvas_Node::Uuid("{B16259BA-9CF6-4143-B09B-5A0F3B4585E6}")
                    ScriptCanvas_Node::Category("String")
                    ScriptCanvas_Node::DynamicSlotOrdering(true)
                    ScriptCanvas_Node::Version(0)
                );

            protected:

                ScriptCanvas_Property(AZStd::string,
                    ScriptCanvas_Property::Name("String", "The resulting string.")
                    ScriptCanvas_Property::Output
                    ScriptCanvas_Property::OutputStorageSpec
                    ScriptCanvas_Property::DisplayGroup("PrintDisplayGroup")
                );

                void OnInputSignal(const SlotId& slotId) override;

            };
        }
    }
}