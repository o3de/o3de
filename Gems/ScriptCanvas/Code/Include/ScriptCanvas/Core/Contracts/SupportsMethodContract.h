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
    //! Contracts that verifies if the specified BehaviorContext method name is supported by a data type.
    //! This can be used to only allow slot connections if the underlying type is able to call the specified method.
    //! For example, container types may support the "Insert" method, while most native or BC types would not.
    class SupportsMethodContract
        : public Contract
    {
    public:
        AZ_CLASS_ALLOCATOR(SupportsMethodContract, AZ::SystemAllocator);
        AZ_RTTI(SupportsMethodContract, "{9C7BD7CB-D11C-4683-8691-F2593D1C294A}", Contract);

        SupportsMethodContract() = default;
        SupportsMethodContract(const char* methodName) 
            : m_methodName(methodName) 
        {}

        ~SupportsMethodContract() override = default;

        static void Reflect(AZ::ReflectContext* reflection);

    protected:

        AZStd::string m_methodName;

        AZ::Outcome<void, AZStd::string> OnEvaluate(const Slot& sourceSlot, const Slot& targetSlot) const override;
        AZ::Outcome<void, AZStd::string> OnEvaluateForType(const Data::Type& dataType) const override;
    };
}
