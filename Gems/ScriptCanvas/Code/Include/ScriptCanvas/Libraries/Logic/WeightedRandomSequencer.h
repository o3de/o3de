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
#include <ScriptCanvas/Core/GraphBus.h>

#include <Include/ScriptCanvas/Libraries/Logic/WeightedRandomSequencer.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            //! Provides a node that uses weighted values to favor execution paths
            class WeightedRandomSequencer
                : public Node
            {

            public:

                SCRIPTCANVAS_NODE(WeightedRandomSequencer);

                static void ReflectDataTypes(AZ::ReflectContext* reflectContext);
            
                WeightedRandomSequencer() = default;
                ~WeightedRandomSequencer() = default;
                
                AZ::Outcome<DependencyReport, void> GetDependencies() const override;

                ConstSlotsOutcome GetSlotsInExecutionThreadByTypeImpl(const Slot& executionSlot, CombinedSlotType targetSlotType, const Slot* /*executionChildSlot*/) const override;

                // Node
                void OnInit() override;
                void ConfigureVisualExtensions() override;

                bool OnValidateNode(ValidationResults& validationResults);
                
                void OnInputSignal(const SlotId& slot);

                SlotId HandleExtension(AZ::Crc32 extensionId) override;

                

                bool CanDeleteSlot(const SlotId& slotId) const override;

                void OnSlotRemoved(const SlotId& slotId) override;
                ////
                
            protected:

                AZ::Crc32 GetWeightExtensionId() const { return AZ_CRC("WRS_Weight_Extension", 0xd17b9467); }
                AZ::Crc32 GetExecutionExtensionId() const { return AZ_CRC("WRS_Execution_Extension", 0x0706035e); }
                AZStd::string GetDisplayGroup() const { return "WeightedExecutionGroup";  }
            
            private:

                struct WeightedPairing
                {
                    AZ_TYPE_INFO(WeightedPairing, "{5D28CA07-95DF-418B-A62C-6B87749DED07}");

                    SlotId m_weightSlotId;
                    SlotId m_executionSlotId;
                };

                struct WeightedStruct
                {
                    int m_totalWeight;
                    SlotId m_executionSlotId;
                };
            
                void RemoveWeightedPair(SlotId slotId);
            
                bool AllWeightsFilled() const;
                bool HasExcessEndpoints() const;
                
                WeightedPairing AddWeightedPair();
                void FixupStateNames();
                
                AZStd::string GenerateDataName(int counter);
                AZStd::string GenerateOutName(int counter);                

                using WeightedPairingList = AZStd::vector<WeightedPairing>;

                WeightedPairingList m_weightedPairings;
            };
        }
    }   
}
