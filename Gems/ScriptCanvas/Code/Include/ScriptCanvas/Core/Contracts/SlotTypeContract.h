/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <ScriptCanvas/Core/Contract.h>


namespace ScriptCanvas
{
    class SlotTypeContract
        : public Contract
    {
    public:
        AZ_CLASS_ALLOCATOR(SlotTypeContract, AZ::SystemAllocator);
        AZ_RTTI(SlotTypeContract, "{084B4F2A-AB34-4931-9269-E3614FC1CDFA}", Contract);

        SlotTypeContract() = default;

        ~SlotTypeContract() override = default;

        static void Reflect(AZ::ReflectContext* reflection);

    protected:
        AZ::Outcome<void, AZStd::string> OnEvaluate(const Slot& sourceSlot, const Slot& targetSlot) const override;
    };
}
