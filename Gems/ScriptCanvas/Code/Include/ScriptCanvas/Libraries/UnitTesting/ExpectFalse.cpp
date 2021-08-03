/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ExpectFalse.h"

#include "UnitTestBus.h"

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace UnitTesting
        {
            void ExpectFalse::OnInit()
            {
                // DYNAMIC_SLOT_VERSION_CONVERTER
                auto candidateSlot = GetSlot(GetSlotId("Candidate"));

                if (candidateSlot && !candidateSlot->IsDynamicSlot())
                {
                    candidateSlot->SetDynamicDataType(DynamicDataType::Any);
                }

                auto referenceSlot = GetSlot(GetSlotId("Reference"));

                if (referenceSlot && !referenceSlot->IsDynamicSlot())
                {
                    referenceSlot->SetDynamicDataType(DynamicDataType::Any);
                }
                ////
            }
        }
    }
}
