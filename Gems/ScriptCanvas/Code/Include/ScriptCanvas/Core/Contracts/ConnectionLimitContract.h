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
#include <ScriptCanvas/Data/Data.h>

namespace ScriptCanvas
{
    class ConnectionLimitContract
        : public Contract
    {
    public:
        AZ_CLASS_ALLOCATOR(ConnectionLimitContract, AZ::SystemAllocator, 0);
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
