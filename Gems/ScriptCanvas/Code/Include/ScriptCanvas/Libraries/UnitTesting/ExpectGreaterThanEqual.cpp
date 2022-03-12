/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ExpectGreaterThanEqual.h"

#include "UnitTestBus.h"
#include "UnitTestBusSenderMacros.h"

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace UnitTesting
        {
            void ExpectGreaterThanEqual::OnInit()
            {
                // DYNAMIC_SLOT_VERSION_CONVERTER
                auto candidateSlot = GetSlot(GetSlotId("Candidate"));

                if (candidateSlot)
                {
                    if (!candidateSlot->IsDynamicSlot())
                    {
                        candidateSlot->SetDynamicDataType(DynamicDataType::Any);
                    }

                    if (candidateSlot->GetDynamicGroup() == AZ::Crc32())
                    {
                        SetDynamicGroup(candidateSlot->GetId(), AZ_CRC("DynamicGroup", 0x219a2e3a));
                    }
                }

                auto referenceSlot = GetSlot(GetSlotId("Reference"));

                if (referenceSlot)
                {
                    if (!referenceSlot->IsDynamicSlot())
                    {
                        referenceSlot->SetDynamicDataType(DynamicDataType::Any);
                    }

                    if (referenceSlot->GetDynamicGroup() == AZ::Crc32())
                    {
                        SetDynamicGroup(referenceSlot->GetId(), AZ_CRC("DynamicGroup", 0x219a2e3a));
                    }
                }
                ////
            }
        }
    }
}
