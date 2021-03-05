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

#include <Include/ScriptCanvas/Libraries/String/Utilities.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace String
        {
            //! Checks if a string starts with a given string, outputs True or False
            class StartsWith
                : public Node
            {
            public:
                ScriptCanvas_Node(StartsWith,
                    ScriptCanvas_Node::Name("Starts With", ".")
                    ScriptCanvas_Node::Uuid("{60EB479A-CF31-4734-B2E5-422828A54A46}")
                    ScriptCanvas_Node::Category("String")
                    ScriptCanvas_Node::Version(0)
                );

            protected:

                // Inputs
                ScriptCanvas_In(ScriptCanvas_In::Name("In", "Input signal"));

                // Outputs
                ScriptCanvas_Out(ScriptCanvas_Out::Name("True", ""));
                ScriptCanvas_Out(ScriptCanvas_Out::Name("False", ""));

                ScriptCanvas_Property(AZStd::string,
                    ScriptCanvas_Property::Name("Source", "The string to search in.")
                    ScriptCanvas_Property::Input
                );

                ScriptCanvas_Property(AZStd::string,
                    ScriptCanvas_Property::Name("Pattern", "The pattern to evaluate.")
                    ScriptCanvas_Property::Input
                );

                ScriptCanvas_PropertyWithDefaults(bool, false,
                    ScriptCanvas_Property::Name("Case Sensitive", "Take into account the case of the string when searching.")
                    ScriptCanvas_Property::Input
                );

                void OnInputSignal(const SlotId& slotId) override;

            };

            //! Checks if a string starts with a given string, outputs True or False
            class EndsWith
                : public Node
            {
            public:
                ScriptCanvas_Node(EndsWith,
                    ScriptCanvas_Node::Name("Ends With", ".")
                    ScriptCanvas_Node::Uuid("{6C1CECA6-C155-4ED5-96BC-1D4F11C7A0FE}")
                    ScriptCanvas_Node::Category("String")
                    ScriptCanvas_Node::Version(0)
                );

            protected:

                // Inputs
                ScriptCanvas_In(ScriptCanvas_In::Name("In", "Input signal"));

                // Outputs
                ScriptCanvas_Out(ScriptCanvas_Out::Name("True", ""));
                ScriptCanvas_Out(ScriptCanvas_Out::Name("False", ""));

                ScriptCanvas_Property(AZStd::string,
                    ScriptCanvas_Property::Name("Source", "The string to search in.")
                    ScriptCanvas_Property::Input
                );

                ScriptCanvas_Property(AZStd::string,
                    ScriptCanvas_Property::Name("Pattern", "The pattern to evaulate.")
                    ScriptCanvas_Property::Input
                );

                ScriptCanvas_PropertyWithDefaults(bool, false,
                    ScriptCanvas_Property::Name("Case Sensitive", "Take into account the case of the string when searching.")
                    ScriptCanvas_Property::Input
                );

                void OnInputSignal(const SlotId& slotId) override;

            };

            //! Splits a string into an array of strings using the specified delimiter (empty space by default).
            //! Returns an array of strings always, if the string cannot be split, the string will be the first
            //! element of the returned array
            class Split
                : public Node
            {
            public:
                ScriptCanvas_Node(Split,
                    ScriptCanvas_Node::Name("Split", ".")
                    ScriptCanvas_Node::Uuid("{327EFC0F-F71E-4028-BAF9-C4223B933FB6}")
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

                static const char* k_defaultDelimiter;
                ScriptCanvas_PropertyWithDefaults(AZStd::string, k_defaultDelimiter,
                    ScriptCanvas_Property::Name("Delimiters", "The characters that can be used as delimiters.")
                    ScriptCanvas_Property::Input
                );

                ScriptCanvas_Property(AZStd::vector<AZStd::string>,
                    ScriptCanvas_Property::Name("String Array", "A container of all the strings.")
                    ScriptCanvas_Property::Output
                    ScriptCanvas_Property::OutputStorageSpec
                );

                void OnInputSignal(const SlotId& slotId) override;

            };

            //! Given an array of strings, this will join the elements in the array and return a single string
            class Join
                : public Node
            {
            public:
                ScriptCanvas_Node(Join,
                    ScriptCanvas_Node::Name("Join", ".")
                    ScriptCanvas_Node::Uuid("{121E5B89-5A8A-4477-A3B7-078B0F1B36FD}")
                    ScriptCanvas_Node::Category("String")
                    ScriptCanvas_Node::Version(0)
                );

            protected:

                // Inputs
                ScriptCanvas_In(ScriptCanvas_In::Name("In", "Input signal"));

                // Outputs
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out", ""));

                ScriptCanvas_Property(AZStd::vector<AZStd::string>,
                    ScriptCanvas_Property::Name("String Array", "The array of strings to join.")
                    ScriptCanvas_Property::Input
                );

                static const char* k_defaultSeparator;
                ScriptCanvas_PropertyWithDefaults(AZStd::string, k_defaultSeparator,
                    ScriptCanvas_Property::Name("Separator", "Will use this string when concatenating the strings from the array.")
                    ScriptCanvas_Property::Input
                );

                ScriptCanvas_Property(AZStd::string,
                    ScriptCanvas_Property::Name("String", "The joined string.")
                    ScriptCanvas_Property::Output
                    ScriptCanvas_Property::OutputStorageSpec
                );

                void OnInputSignal(const SlotId& slotId) override;

            };

        }
    }
}
