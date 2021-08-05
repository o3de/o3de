/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/SlotConfigurations.h>

namespace ScriptCanvas
{
    namespace CommonSlots
    {
        struct GeneralInSlot
            : public ExecutionSlotConfiguration
        {
            static constexpr const char* GetName() { return "In"; }
            
            GeneralInSlot()
                : ExecutionSlotConfiguration(GetName(), ConnectionType::Input)
            {
            }            
        };
        
        struct GeneralOutSlot
            : public ExecutionSlotConfiguration
        {
            static constexpr const char* GetName() { return "Out"; }
            
            GeneralOutSlot()
                : ExecutionSlotConfiguration(GetName(), ConnectionType::Output)
            {
            }
        };        
    }    
}
