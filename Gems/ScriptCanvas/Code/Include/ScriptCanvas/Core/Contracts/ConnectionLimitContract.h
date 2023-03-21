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
    class ConnectionLimitContract
        : public Contract
    {
    public:
        AZ_CLASS_ALLOCATOR(ConnectionLimitContract, AZ::SystemAllocator);
        AZ_RTTI(ConnectionLimitContract, "{C66FB68F-63D5-4EE2-BC28-D566EC2E5159}", Contract);

        ConnectionLimitContract(AZ::s32 limit = -1);
        ~ConnectionLimitContract() override = default;

        static void Reflect(AZ::ReflectContext* reflection);

        void SetLimit(AZ::s32 limit);

    protected:
        AZ::s32 m_limit;

        AZ::Outcome<void, AZStd::string> OnEvaluate(const Slot& sourceSlot, const Slot& targetSlot) const override;
    };
}
