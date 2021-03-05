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

#include <Libraries/Core/BinaryOperator.h>
#include <Libraries/Math/ArithmeticFunctions.h>

#include <Include/ScriptCanvas/Libraries/Math/Random.generated.h>


namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Math
        {
            class Random
                : public Node
            {
            public:

                ScriptCanvas_Node(Random,
                    ScriptCanvas_Node::Deprecated("This node has been deprecated, please use one of the nodes in the Random category instead")
                    ScriptCanvas_Node::EditAttributes(AZ::Script::Attributes::ExcludeFrom(AZ::Script::Attributes::ExcludeFlags::All))
                    ScriptCanvas_Node::Name("Random")
                    ScriptCanvas_Node::Category("Math/Random/Deprecated")
                    ScriptCanvas_Node::Uuid("{7884F790-EA26-49AE-9168-D4C415C0D9C3}")
                    ScriptCanvas_Node::Description("Provides a random number in the range specified")
                );

                // Inputs
                ScriptCanvas_In(ScriptCanvas_In::Name("In", "When signaled, execution is delayed at this node according to the specified properties.")
                    ScriptCanvas_In::Contracts({ DisallowReentrantExecutionContract }));

                // Outputs
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out", "Signaled when the delay reaches zero."));

                // Data
                ScriptCanvas_PropertyWithDefaults(float, 0.f,
                    ScriptCanvas_Property::Name("Min", "")
                    ScriptCanvas_Property::Input);

                ScriptCanvas_PropertyWithDefaults(float, 1.f,
                    ScriptCanvas_Property::Name("Max", "")
                    ScriptCanvas_Property::Input);

                ScriptCanvas_Property(float,
                    ScriptCanvas_Property::Name("Result", ".") ScriptCanvas_Property::Visibility(false)
                    ScriptCanvas_Property::Output
                    ScriptCanvas_Property::OutputStorageSpec
                );


                void OnInputSignal(const SlotId& slotId);

            };

#if defined(EXPRESSION_TEMPLATES_ENABLED)

            class Random
                : public BinaryOperatorGeneric<Random, ArithmeticOperator<OperatorType::Random>>
            {
            public:
                using BaseType = BinaryOperatorGeneric<Random, ArithmeticOperator<OperatorType::Random>>;
                AZ_COMPONENT(Random, "{E102960E-9DA6-4C8D-B634-2F651BA5EDDC}", BaseType);

                static const char* GetOperatorName() { return "Random"; }
                static const char* GetOperatorDesc() { return "Generate a random number between two numbers"; }
                static const char* GetIconPath() { return "Editor/Icons/ScriptCanvas/Random.png"; }

                
            };
#endif//defined(EXPRESSION_TEMPLATES_ENABLED)
        }
    }
}

