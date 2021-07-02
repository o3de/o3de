/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Start.h"

#include <ScriptCanvas/Core/GraphBus.h>
#include <ScriptCanvas/Grammar/Primitives.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            void Start::OnInputSignal(const SlotId&)
            {
                SignalOutput(StartProperty::GetOutSlotId(this));
            }
       }
    }
}
