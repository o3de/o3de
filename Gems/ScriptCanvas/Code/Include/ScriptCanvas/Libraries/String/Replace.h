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

#include <Include/ScriptCanvas/Libraries/String/Replace.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace String
        {
            //! A String formatting class that produces a data output based on the specified string format and
            //! input values.
            class Replace
                : public Node
            {
            public:
                ScriptCanvas_Node(Replace,
                    ScriptCanvas_Node::Name("Replace String", "Allows replacing a substring from a given string.")
                    ScriptCanvas_Node::Uuid("{197D0BAA-FCAF-4922-872B-3A95BEA574B2}")
                    ScriptCanvas_Node::Category("String")
                    ScriptCanvas_Node::Version(0)
                );

            protected:

                // Inputs
                ScriptCanvas_In(ScriptCanvas_In::Name("In", "Input signal"));

                // Outputs
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out", ""));

                ScriptCanvas_Property(AZStd::string,
                    ScriptCanvas_Property::Name("Source", "The string to search in.")
                    ScriptCanvas_Property::Input
                );

                ScriptCanvas_Property(AZStd::string,
                    ScriptCanvas_Property::Name("Replace", "The substring to search for.")
                    ScriptCanvas_Property::Input
                );

                ScriptCanvas_Property(AZStd::string,
                    ScriptCanvas_Property::Name("With", "The string to replace the substring with.")
                    ScriptCanvas_Property::Input
                );

                ScriptCanvas_PropertyWithDefaults(bool, true,
                    ScriptCanvas_Property::Name("Case Sensitive", "Take into account the case of the string when searching.")
                    ScriptCanvas_Property::Input
                );

                ScriptCanvas_Property(AZStd::string,
                    ScriptCanvas_Property::Name("Result", "The resulting string.")
                    ScriptCanvas_Property::Output
                    ScriptCanvas_Property::OutputStorageSpec
                );


                void OnInputSignal(const SlotId& slotId) override;

            };
        }
    }
}
