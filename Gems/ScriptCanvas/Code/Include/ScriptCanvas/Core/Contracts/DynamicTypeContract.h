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
    class DynamicTypeContract
        : public Contract
    {
    public:
        AZ_CLASS_ALLOCATOR(DynamicTypeContract, AZ::SystemAllocator);
        AZ_RTTI(DynamicTypeContract, "{00822E5B-7DD0-4D52-B1A8-9CE9C1A5C4FB}", Contract);
        
        ~DynamicTypeContract() override = default;

        static void Reflect(AZ::ReflectContext* reflection);
        
    protected:
        AZ::Outcome<void, AZStd::string> OnEvaluate(const Slot& sourceSlot, const Slot& targetSlot) const override;
    };
}
