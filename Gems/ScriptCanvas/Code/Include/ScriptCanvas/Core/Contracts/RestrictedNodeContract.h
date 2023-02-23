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
    class RestrictedNodeContract
        : public Contract
    {
    public:
        AZ_CLASS_ALLOCATOR(RestrictedNodeContract, AZ::SystemAllocator);
        AZ_RTTI(RestrictedNodeContract, "{DC2B464E-17EE-4CAC-89E9-84C76605E766}", Contract);

        using Contract::Contract;
        RestrictedNodeContract() = default;
        RestrictedNodeContract(AZ::EntityId nodeId);

        static void Reflect(AZ::ReflectContext* reflection);

        void SetNodeId(AZ::EntityId nodeId);

    protected:
        AZ::Outcome<void, AZStd::string> OnEvaluate(const Slot& sourceSlot, const Slot& targetSlot) const override;

    private:
        AZ::EntityId m_nodeId;
    };
}
