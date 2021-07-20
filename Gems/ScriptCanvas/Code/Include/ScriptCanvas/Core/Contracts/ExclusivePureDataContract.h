/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
