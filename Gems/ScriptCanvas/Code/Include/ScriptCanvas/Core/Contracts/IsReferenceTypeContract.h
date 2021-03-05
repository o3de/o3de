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
    class IsReferenceTypeContract
        : public Contract
    {
    public:
        AZ_CLASS_ALLOCATOR(IsReferenceTypeContract, AZ::SystemAllocator, 0);
        AZ_RTTI(IsReferenceTypeContract, "{7BBA9F9A-AABF-458F-B5D6-B7CCDC8C9BE6}", Contract);
        
        ~IsReferenceTypeContract() override = default;

        static void Reflect(AZ::ReflectContext* reflection);
        
    protected:
        AZ::Outcome<void, AZStd::string> OnEvaluate(const Slot& sourceSlot, const Slot& targetSlot) const override;
        AZ::Outcome<void, AZStd::string> OnEvaluateForType(const Data::Type& dataType) const override;
    };
}
