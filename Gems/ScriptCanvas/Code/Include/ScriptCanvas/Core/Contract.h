/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/functional.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvas
{
    class Slot;
    class Contract;

    namespace Data
    {
        class Type;
    }

    //! Function which will be invoked when a slot is created to allow the creation of a slot contract object
    using ContractCreationFunction = AZStd::function<Contract*()>;
    struct ContractDescriptor
    {
        AZ_CLASS_ALLOCATOR(ContractDescriptor, AZ::SystemAllocator);
        AZ_TYPE_INFO(ContractDescriptor, "{C0E3537F-5E6A-4269-A717-17089559F7A1}");
        ContractCreationFunction m_createFunc;

        ContractDescriptor() = default;

        ContractDescriptor(ContractCreationFunction&& createFunc)
            : m_createFunc(AZStd::move(createFunc))
        {}
    };

    class Contract
    {
    public:
        AZ_CLASS_ALLOCATOR(Contract, AZ::SystemAllocator);
        AZ_RTTI(Contract, "{93846E60-BD7E-438A-B970-5C4AA591CF93}");

        Contract() = default;
        virtual ~Contract() = default;

        static void Reflect(AZ::ReflectContext* reflection);

        AZ::Outcome<void, AZStd::string> Evaluate(const Slot& sourceSlot, const Slot& targetSlot) const;
        AZ::Outcome<void, AZStd::string> EvaluateForType(const Data::Type& dataType) const;

    protected:
        virtual AZ::Outcome<void, AZStd::string> OnEvaluate(const Slot& sourceSlot, const Slot& targetSlot) const = 0;

        // By default accept all Data::Types for each contract
        // Mainly here for legacy support new contracts should implement this themselves.
        virtual AZ::Outcome<void, AZStd::string> OnEvaluateForType(const Data::Type& dataType) const
        {
            AZ_UNUSED(dataType);
            return AZ::Success();
        };
    };
}
