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
    class ExclusivePureDataContract
        : public Contract
    {
    public:
        AZ_CLASS_ALLOCATOR(ExclusivePureDataContract, AZ::SystemAllocator, 0);
        AZ_RTTI(ExclusivePureDataContract, "{E48A0B26-B6B7-4AF3-9341-9E5C5C1F0DE8}", Contract);

        // no multiple literals, variables, defaults, gets, or any other new form of data that can be routed without getting pushed by execution
        ~ExclusivePureDataContract() override = default;

        static void Reflect(AZ::ReflectContext* reflection);

    protected:
        AZ::Outcome<void, AZStd::string> OnEvaluate(const Slot& sourceSlot, const Slot& targetSlot) const override;

        AZ::Outcome<void, AZStd::string> HasNoPureDataConnection(const Slot& dataInputSlot) const;
    };
}
