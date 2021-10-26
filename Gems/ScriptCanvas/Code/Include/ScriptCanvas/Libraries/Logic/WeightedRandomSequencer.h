/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

                bool OnValidateNode(ValidationResults& validationResults) override;

                SlotId HandleExtension(AZ::Crc32 extensionId) override;

                bool CanDeleteSlot(const SlotId& slotId) const override;

                void OnSlotRemoved(const SlotId& slotId) override;
                ////

            protected:

                AZ::Crc32 GetWeightExtensionId() const { return AZ_CRC("WRS_Weight_Extension", 0xd17b9467); }
                AZ::Crc32 GetExecutionExtensionId() const { return AZ_CRC("WRS_Execution_Extension", 0x0706035e); }
                AZStd::string GetDisplayGroup() const { return "WeightedExecutionGroup"; }

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
