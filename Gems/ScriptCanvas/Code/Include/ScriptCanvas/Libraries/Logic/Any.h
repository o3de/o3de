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

#include <AzCore/std/containers/vector.h>

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/GraphBus.h>

#include <ScriptCanvas/CodeGen/CodeGen.h>

#include <Include/ScriptCanvas/Libraries/Logic/Any.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            class Any
                : public Node
            {
                ScriptCanvas_Node(Any,
                    ScriptCanvas_Node::Uuid("{6359C34F-3C34-4784-9FFD-18F1C8E3482D}")
                    ScriptCanvas_Node::Description("Will trigger the Out pin whenever any of the In pins get triggered")
                    ScriptCanvas_Node::Version(1, AnyNodeVersionConverter)
                );

                enum Version
                {
                    InitialVersion = 0,
                    RemoveInputsContainers,

                    Current
                };
                
            public:

                static bool AnyNodeVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement);

                Any() = default;

                // Node
                void OnInit() override;
                void ConfigureVisualExtensions() override;

                void OnInputSignal(const SlotId& slot) override;

                SlotId HandleExtension(AZ::Crc32 extensionId) override;

                bool CanDeleteSlot(const SlotId& slotId) const override;

                void OnSlotRemoved(const SlotId& slotId) override;
                ////
    
            protected:

                AZ::Crc32 GetInputExtensionId() const { return AZ_CRC("Output", 0xccde149e); }

                // Outputs
                ScriptCanvas_Out(ScriptCanvas_Out::Name("Out", "Signaled when the node receives a signal from the selected index"));
                
            private:

                AZStd::string GenerateInputName(int counter);

                SlotId AddInputSlot();
                void FixupStateNames();
            };
        }
    }
}