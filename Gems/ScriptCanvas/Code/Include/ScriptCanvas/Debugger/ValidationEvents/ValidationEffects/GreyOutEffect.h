/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Component/EntityId.h>

namespace ScriptCanvas
{
    class GreyOutNodeEffect
    {
    public:
        AZ_RTTI(GreyOutNodeEffect, "{422124DF-B5B9-4450-874B-D1DD9AA50E98}");
        AZ_CLASS_ALLOCATOR(GreyOutNodeEffect, AZ::SystemAllocator);

        GreyOutNodeEffect() = default;
        virtual ~GreyOutNodeEffect() = default;

        virtual AZ::EntityId GetGreyOutNodeId() const = 0;
    };
}
