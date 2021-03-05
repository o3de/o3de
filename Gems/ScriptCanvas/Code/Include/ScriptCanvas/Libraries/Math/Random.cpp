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
