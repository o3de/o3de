/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Random.h"
#include <Libraries/Math/MathNodeUtilities.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Math
        {
         
            void Random::OnInputSignal(const SlotId& slotId)
            {
                if (slotId == RandomProperty::GetInSlotId(this))
                {
                    const SlotId resultSlotId = RandomProperty::GetResultSlotId(this);

                    if (Slot* resultSlot = GetSlot(resultSlotId))
                    {
                        float minValue = RandomProperty::GetMin(this);
                        float maxValue = RandomProperty::GetMax(this);

                        auto randVal = MathNodeUtilities::GetRandom(minValue, maxValue);

                        Datum o(Data::Type::Number(), Datum::eOriginality::Copy);
                        o.Set(randVal);
                        PushOutput(o, *resultSlot);
                    }
                }

                SignalOutput(RandomProperty::GetOutSlotId(this));
            }

        }
    }
}
