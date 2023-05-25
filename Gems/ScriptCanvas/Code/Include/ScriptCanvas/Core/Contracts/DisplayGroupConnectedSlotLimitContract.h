/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <ScriptCanvas/Core/Contract.h>
#include <ScriptCanvas/Data/Data.h>

namespace ScriptCanvas
{
    // Limits the number of slots that can have connections within a given display group.
    class DisplayGroupConnectedSlotLimitContract
        : public Contract
    {
    public:
        AZ_CLASS_ALLOCATOR(DisplayGroupConnectedSlotLimitContract, AZ::SystemAllocator);
        AZ_RTTI(DisplayGroupConnectedSlotLimitContract, "{71E55CC5-6212-48C2-973E-1AC9E20A4481}", Contract);
        
        static void Reflect(AZ::ReflectContext* reflection);

        DisplayGroupConnectedSlotLimitContract() = default;
        DisplayGroupConnectedSlotLimitContract(const AZStd::string& displayGroup, AZ::u32 connectionLimit);
        ~DisplayGroupConnectedSlotLimitContract() override = default;

        void SetDisplayGroup(const AZStd::string& displayGroup)
        {
            m_displayGroup = displayGroup;
        }

        void SetConnectionLimit(AZ::u32 connectionLimit)
        {
            m_limit = connectionLimit;
        }
        
        void SetCustomErrorMessage(const AZStd::string& customErrorMessage)
        {
            m_customErrorMessage = customErrorMessage;
        }

    protected:
    
        AZStd::string m_displayGroup;
        AZ::u32 m_limit = 0;
        
        AZStd::string m_customErrorMessage;

        AZ::Outcome<void, AZStd::string> OnEvaluate(const Slot& sourceSlot, const Slot& targetSlot) const override;
    };
}
