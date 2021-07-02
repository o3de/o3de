/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "While.h"

#include <ScriptCanvas/Execution/ErrorBus.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            AZ::Outcome<DependencyReport, void> While::GetDependencies() const
            {
                return AZ::Success(DependencyReport{});
            }
            
            SlotId While::GetLoopFinishSlotId() const
            {
                return WhileProperty::GetOutSlotId(const_cast<While*>(this));
            }

            SlotId While::GetLoopSlotId() const
            {
                return WhileProperty::GetLoopSlotId(const_cast<While*>(this));
            }

            bool While::IsFormalLoop() const
            { 
                return true; 
            }
        }
    }
}
