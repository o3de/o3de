/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <ScriptCanvas/Core/Contract.h>
#include <ScriptCanvas/Data/Data.h>

namespace ScriptCanvas
{
    class MathOperatorContract
        : public Contract
    {
    public:
        AZ_CLASS_ALLOCATOR(MathOperatorContract, AZ::SystemAllocator);
        AZ_RTTI(MathOperatorContract, "{17B1AEA6-B36B-4EE5-83E9-4563CAC79889}", Contract);

        MathOperatorContract() = default;
        MathOperatorContract(AZStd::string_view operatorMethod)
            : m_supportedOperator(operatorMethod)
        {
        }

        ~MathOperatorContract() override = default;        

        void SetSupportedNativeTypes(const AZStd::unordered_set< Data::Type >& nativeTypes);
        void SetSupportedOperator(AZStd::string_view operatorString);

        // Was a versioning mishap with the data in this contract that caused the supported
        // types and operator to not be serialized. Using this to catch that case and update.
        // Function will be removed in a future update.
        bool HasOperatorFunction() const;

        static void Reflect(AZ::ReflectContext* reflection);

    protected:

        AZStd::string m_supportedOperator;
        AZStd::unordered_set< Data::Type > m_supportedNativeTypes;

        AZ::Outcome<void, AZStd::string> OnEvaluate(const Slot& sourceSlot, const Slot& targetSlot) const override;
        AZ::Outcome<void, AZStd::string> OnEvaluateForType(const Data::Type& dataType) const override;
    };
}
