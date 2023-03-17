/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/vector.h>

#include <ScriptCanvas/Core/Contract.h>
#include <ScriptCanvas/Data/Data.h>

namespace ScriptCanvas
{
    class ContractRTTI
        : public Contract
    {
    public:
        AZ_CLASS_ALLOCATOR(ContractRTTI, AZ::SystemAllocator);
        AZ_RTTI(ContractRTTI, "{3CB87E9B-33A0-40B1-A7CC-72465814BEE6}", Contract);

        enum Flags
        {
            Inclusive, //> Contract will be satisfied by any of the type Uuids stored in the contract.
            Exclusive, //> Contract may satisfy any endpoint except to those types in the contract.
        };

        ContractRTTI(Flags flags = Inclusive) : m_flags(flags) {}
        ContractRTTI(std::initializer_list<AZ::Uuid> types, Flags flags = Inclusive)
            : ContractRTTI(types.begin(), types.end(), flags)
        {}

        template<typename Container>
        ContractRTTI(const Container& types, Flags flags = Inclusive)
            : ContractRTTI(types.begin(), types.end(), flags)
        {}

        template<typename InputIterator>
        ContractRTTI(InputIterator first, InputIterator last, Flags flags = Inclusive)
            : m_types(first, last)
            , m_flags(flags)
        {}

        ~ContractRTTI() override = default;

        static void Reflect(AZ::ReflectContext* reflection);

        void AddType(const AZ::Uuid& type) { m_types.push_back(type); }

    protected:
        Flags m_flags;
        AZStd::vector<AZ::Uuid> m_types;

        AZ::Outcome<void, AZStd::string> OnEvaluate(const Slot& sourceSlot, const Slot& targetSlot) const override;
    };
}
