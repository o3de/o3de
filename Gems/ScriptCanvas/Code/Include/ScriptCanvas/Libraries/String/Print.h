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

#include <Include/ScriptCanvas/Libraries/String/Print.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace String
        {
            //! Prints a formatted string into the console.
            class Print
                : public Internal::StringFormatted
            {
            public:
                ScriptCanvas_Node(Print,
                    ScriptCanvas_Node::Name("Print", "Formats and prints the provided text in the debug console.\nAny word within {} will create a data pin on this node.")
                    ScriptCanvas_Node::Uuid("{E1940FB4-83FE-4594-9AFF-375FF7603338}")
                    ScriptCanvas_Node::Category("Utilities/Debug")
                    ScriptCanvas_Node::EditAttributes(AZ::Edit::Attributes::CategoryStyle(".string"), ScriptCanvas::Attributes::Node::TitlePaletteOverride("StringNodeTitlePalette"))
                    ScriptCanvas_Node::DynamicSlotOrdering(true)
                    ScriptCanvas_Node::Version(0)
                );

            protected:

                void OnInputSignal(const SlotId&) override;

            };
        }
    }
}

