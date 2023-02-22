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
    class IsReferenceTypeContract
        : public Contract
    {
    public:
        AZ_CLASS_ALLOCATOR(IsReferenceTypeContract, AZ::SystemAllocator);
        AZ_RTTI(IsReferenceTypeContract, "{7BBA9F9A-AABF-458F-B5D6-B7CCDC8C9BE6}", Contract);
        
        ~IsReferenceTypeContract() override = default;

        static void Reflect(AZ::ReflectContext* reflection);
        
    protected:
        AZ::Outcome<void, AZStd::string> OnEvaluate(const Slot& sourceSlot, const Slot& targetSlot) const override;
        AZ::Outcome<void, AZStd::string> OnEvaluateForType(const Data::Type& dataType) const override;
    };
}
