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

#include <ScriptCanvas/Core/Node.h>

#include <Include/ScriptCanvas/Libraries/Logic/TargetedSequencer.generated.h>


namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            // Will trigger the specified execution upon being triggered
            class TargetedSequencer
                : public Node
            {
                SCRIPTCANVAS_NODE(TargetedSequencer);

            public:

                TargetedSequencer();

                void OnInit() override;
                void OnConfigured() override;
                void ConfigureVisualExtensions() override;
                
                bool CanDeleteSlot(const SlotId& slotId) const;

                SlotId HandleExtension(AZ::Crc32 extensionId);        

                // Script Canvas Translation...
                bool IsSupportedByNewBackend() const override { return true; }

                bool IsSwitchStatement() const override { return true; }

                AZ::Outcome<DependencyReport, void> GetDependencies() const override
                {
                    return AZ::Success(DependencyReport{});
                }

                SlotsOutcome GetSlotsInExecutionThreadByTypeImpl(const Slot&, CombinedSlotType targetSlotType, const Slot*) const override
                {
                    return AZ::Success(GetSlotsByType(targetSlotType));
                }
                //////////////////////////////////////////////////////////////////////////

            protected:
            
                AZStd::string GetDisplayGroup() const { return "OutputGroup"; }

                void OnInputSignal(const SlotId& slot) override;                
                void OnSlotRemoved(const SlotId& slotId) override;

            private:

                AZStd::string GenerateOutputName(int counter);
                void FixupStateNames();
                
                int m_numOutputs;
            };
        }
    }
}
