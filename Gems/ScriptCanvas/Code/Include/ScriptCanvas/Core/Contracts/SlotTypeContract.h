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
#pragma once

#include <ScriptCanvas/Core/Contract.h>


namespace ScriptCanvas
{
    class SlotTypeContract
        : public Contract
    {
    public:
        AZ_CLASS_ALLOCATOR(SlotTypeContract, AZ::SystemAllocator, 0);
        AZ_RTTI(SlotTypeContract, "{084B4F2A-AB34-4931-9269-E3614FC1CDFA}", Contract);

        SlotTypeContract() = default;

        ~SlotTypeContract() override = default;

        static void Reflect(AZ::ReflectContext* reflection);

    protected:
        AZ::Outcome<void, AZStd::string> OnEvaluate(const Slot& sourceSlot, const Slot& targetSlot) const override;
    };
}
