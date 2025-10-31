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

        struct FloatData : public DataSlotConfiguration
        {
            FloatData(AZStd::string_view name, ConnectionType connectionType, bool isLatent = false)
                : DataSlotConfiguration(Data::Type::Number(), name, connectionType)
            {
                m_isLatent = isLatent;
            }
        };

        struct Execution : public ExecutionSlotConfiguration
        {
            Execution(AZStd::string_view name, ConnectionType connectionType, bool isLatent = false, bool createsImplicitConnections = false)
                : ExecutionSlotConfiguration()
            {
                m_name = name;
                SetConnectionType(connectionType);
                m_isLatent = isLatent;
                m_createsImplicitConnections = createsImplicitConnections;
            }
        };
    }    
}
