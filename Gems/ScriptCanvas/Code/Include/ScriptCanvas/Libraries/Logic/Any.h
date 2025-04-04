/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/vector.h>

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/GraphBus.h>

#include <Include/ScriptCanvas/Libraries/Logic/Any.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            //! Will trigger the Out pin whenever any of the In pins get triggered
            class Any
                : public Node
            {
                enum Version
                {
                    InitialVersion = 0,
                    RemoveInputsContainers,

                    Current
                };

            public:

                SCRIPTCANVAS_NODE(Any);


                static bool AnyNodeVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement);

                Any() = default;

                // Node...
                void OnInit() override;
                void ConfigureVisualExtensions() override;

                SlotId HandleExtension(AZ::Crc32 extensionId) override;

                bool CanDeleteSlot(const SlotId& slotId) const override;

                void OnSlotRemoved(const SlotId& slotId) override;
                ////

                /// Translation
                bool IsNoOp() const override;

                AZ::Outcome<DependencyReport, void> GetDependencies() const override;

            protected:
                ConstSlotsOutcome GetSlotsInExecutionThreadByTypeImpl(const Slot&, CombinedSlotType targetSlotType, const Slot*) const override
                {
                    return AZ::Success(GetSlotsByType(targetSlotType));
                }

                AZ::Crc32 GetInputExtensionId() const { return AZ_CRC_CE("Output"); }

            private:
                AZStd::string GenerateInputName(int counter);
                SlotId AddInputSlot();
                void FixupStateNames();
            };
        }
    }
}
