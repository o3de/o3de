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

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Component/EntityId.h>

namespace ScriptCanvas
{
    class GreyOutNodeEffect
    {
    public:
        AZ_RTTI(GreyOutNodeEffect, "{422124DF-B5B9-4450-874B-D1DD9AA50E98}");
        AZ_CLASS_ALLOCATOR(GreyOutNodeEffect, AZ::SystemAllocator, 0);

        GreyOutNodeEffect() = default;
        virtual ~GreyOutNodeEffect() = default;

        virtual AZ::EntityId GetGreyOutNodeId() const = 0;
    };
}
