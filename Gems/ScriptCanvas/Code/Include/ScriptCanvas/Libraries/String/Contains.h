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

#include <Include/ScriptCanvas/Libraries/String/Contains.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace String
        {
            //! Checks if the specified substring exists in the provided string
            class Contains
                : public Node
            {
            public:

                ScriptCanvas_Node(Contains,
                    ScriptCanvas_Node::Name("Contains String", "Checks if a string contains an instance of a specified string, if true, it returns the index to the first instance matched.")
                    ScriptCanvas_Node::Uuid("{8481E892-DE37-4CCF-86AA-E4770DE90643}")
                    ScriptCanvas_Node::Category("String")
                    ScriptCanvas_Node::Version(0)
                );

            protected:

                // Inputs
                ScriptCanvas_In(ScriptCanvas_In::Name("In", "Input signal"));

                ScriptCanvas_Property(AZStd::string,
                    ScriptCanvas_Property::Name("Source", "The string to search in.")
                    ScriptCanvas_Property::Input
                );

                ScriptCanvas_Property(AZStd::string,
                    ScriptCanvas_Property::Name("Pattern", "The substring to search for.")
                    ScriptCanvas_Property::Input
                );

                ScriptCanvas_Property(bool,
                    ScriptCanvas_Property::Name("Search From End", "Start the match checking from the end of a string.")
                    ScriptCanvas_Property::Input
                );

                ScriptCanvas_PropertyWithDefaults(bool, true,
                    ScriptCanvas_Property::Name("Case Sensitive", "Take into account the case of the string when searching.")
                    ScriptCanvas_Property::Input
                );

                ScriptCanvas_Out(ScriptCanvas_Out::Name("True", "The string contains the provided pattern."));
                ScriptCanvas_Out(ScriptCanvas_Out::Name("False", "The string did not contain the provided pattern."));
                
                
                ScriptCanvas_Property(int,
                    ScriptCanvas_Property::Name("Index", "The first index at which the substring was found.")
                    ScriptCanvas_Property::Output,
                    ScriptCanvas_Property::OutputStorageSpec
                );

                void OnInputSignal(const SlotId& slotId) override;

            };
        }
    }
}
