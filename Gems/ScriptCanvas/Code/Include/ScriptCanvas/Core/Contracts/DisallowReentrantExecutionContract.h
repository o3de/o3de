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
    class DisallowReentrantExecutionContract
        : public Contract
    {
    public:
        AZ_CLASS_ALLOCATOR(DisallowReentrantExecutionContract, AZ::SystemAllocator);
        AZ_RTTI(DisallowReentrantExecutionContract, "{8B476D16-D11C-4274-BE61-FA9B34BF54A3}", Contract);

        DisallowReentrantExecutionContract() = default;

        ~DisallowReentrantExecutionContract() override = default;

        static void Reflect(AZ::ReflectContext* reflection);

    protected:
        AZ::Outcome<void, AZStd::string> OnEvaluate(const Slot& sourceSlot, const Slot& targetSlot) const override;
    };
}
